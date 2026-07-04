import os
import sys
import pathlib
import ctypes

if sys.platform == "win32":
    _root = pathlib.Path(__file__).parent.parent.absolute()
    os.add_dll_directory(str(_root / "ClangLib" / "ClangDLLs"))
    os.add_dll_directory(str(_root))

from cinpy_core import (
    _init_clang,
    _parse_header,
    _gen_cpyc,
    _load_cpyc,
    _ir_fn_list,
    _call_fn,
    _resolve_fn_record,
    _InternalFunction,
    _cache_evict,
    _config,
    CinPyError,
)
from cinpy._proxy    import HeaderProxy
from cinpy._resolver import resolve_header
from cinpy._libproxy import LibProxy
from cinpy._libloader import load_library
from cinpy._conly    import COnly

_clang_initialized = False
_CPYC_DIR = pathlib.Path.cwd() / "__cpyc__"


def _ensure_clang():
    global _clang_initialized
    if _clang_initialized:
        return

    dll = os.environ.get("CINPY_LIBCLANG", "")
    if dll:
        _init_clang(dll)
        _clang_initialized = True
        return

    _pkg = pathlib.Path(__file__).parent.parent.absolute()

    candidates = [
        _pkg / "ClangLib" / "ClangDLLs" / "libclang.dll",
        pathlib.Path("/usr/lib/x86_64-linux-gnu/libclang.so"),
        pathlib.Path("/usr/lib/x86_64-linux-gnu/libclang.so.1"),
        pathlib.Path("/usr/lib/llvm-18/lib/libclang.so"),
        pathlib.Path("/usr/lib/llvm-17/lib/libclang.so"),
        pathlib.Path("/usr/lib/llvm-16/lib/libclang.so"),
        pathlib.Path("/usr/lib/llvm-15/lib/libclang.so"),
        pathlib.Path("/usr/lib/llvm-14/lib/libclang.so"),
        pathlib.Path("/usr/lib/aarch64-linux-gnu/libclang.so"),
        pathlib.Path("/usr/lib/aarch64-linux-gnu/libclang.so.1"),
        pathlib.Path("/opt/homebrew/opt/llvm/lib/libclang.dylib"),
        pathlib.Path("/usr/local/opt/llvm/lib/libclang.dylib"),
        pathlib.Path("/Library/Developer/CommandLineTools/usr/lib/libclang.dylib"),
        pathlib.Path("/Applications/Xcode.app/Contents/Developer/Toolchains/"
                     "XcodeDefault.xctoolchain/usr/lib/libclang.dylib"),
    ]

    for c in candidates:
        if c.exists():
            _init_clang(str(c))
            _clang_initialized = True
            return

    raise CinPyError(
        "libclang not found.\n"
        "  Windows: bundled in ClangLib\\ClangDLLs\\ — reinstall CinPy.\n"
        "  Linux:   sudo apt install libclang-dev\n"
        "           or: sudo dnf install clang-devel\n"
        "           or: sudo pacman -S clang\n"
        "  macOS:   brew install llvm\n"
        "  Any:     set CINPY_LIBCLANG=/path/to/libclang.so"
    )


def _cpyc_path(header):
    stem = pathlib.Path(header).name.replace(".", "_")
    return _CPYC_DIR / f"{stem}.cpyc"


def _resolve_cpyc(header):
    p = _cpyc_path(header)
    return p if p.exists() else None


def LoadHeader(header, aot=False, FullPath=False, FullPathLoc=None):
    resolved  = resolve_header(header, full_path=FullPath, full_path_loc=FullPathLoc)
    _ensure_clang()
    clang_inc = str(pathlib.Path(__file__).parent.parent / "ClangHeaders")
    _CPYC_DIR.mkdir(exist_ok=True)

    if aot:
        ir_cap = _parse_header(resolved, clang_inc)
        _gen_cpyc(ir_cap, str(_cpyc_path(header)))
        return HeaderProxy(ir_cap)

    existing = _resolve_cpyc(header)
    if existing:
        ir_cap = _load_cpyc(str(existing))
    else:
        ir_cap = _parse_header(resolved, clang_inc)
        _gen_cpyc(ir_cap, str(_cpyc_path(header)))
    return HeaderProxy(ir_cap)


def LoadLibrary(name, aot=False):
    resolved = load_library(name)
    is_win   = sys.platform == "win32"

    if is_win:
        import ctypes.wintypes
        _kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        handle = _kernel32.LoadLibraryA(resolved.encode())
        if not handle:
            err = ctypes.get_last_error()
            raise OSError(
                f"CinPy LoadLibrary: failed to load '{resolved}' (error {err})"
            )
    else:
        handle = ctypes.CDLL(resolved)
        if not handle:
            raise OSError(f"CinPy LoadLibrary: failed to load '{resolved}'")

    return LibProxy(handle, resolved, is_win)


def GenCPyC(header, out_path=None, FullPath=False, FullPathLoc=None):
    resolved  = resolve_header(header, full_path=FullPath, full_path_loc=FullPathLoc)
    _ensure_clang()
    clang_inc = str(pathlib.Path(__file__).parent.parent / "ClangHeaders")
    ir_cap    = _parse_header(resolved, clang_inc)
    dest      = out_path or str(_cpyc_path(header))
    _CPYC_DIR.mkdir(exist_ok=True)
    _gen_cpyc(ir_cap, dest)
    return dest


def LoadCPyC(cpyc_path):
    ir_cap = _load_cpyc(cpyc_path)
    return HeaderProxy(ir_cap)


def config(cache_size=0, cache_timeout=0):
    _config(cache_size, cache_timeout)


def evict():
    _cache_evict()