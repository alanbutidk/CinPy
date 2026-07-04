import sys
import pathlib
import ctypes
import os

_ROOT = pathlib.Path(__file__).parent.parent.absolute()

if sys.platform == "win32":
    _CLANGLIB_DIR = _ROOT / "ClangLib" / "ClangDLLs"
elif sys.platform == "darwin":
    _CLANGLIB_DIR = _ROOT / "ClangLib" / "ClangDYLibs"
else:
    _CLANGLIB_DIR = _ROOT / "ClangLib" / "ClangSOs"


def _normalize_libname(name):
    p = pathlib.Path(name)
    if p.suffix in (".dll", ".so", ".dylib"):
        return name
    if sys.platform == "win32":
        return name + ".dll"
    elif sys.platform == "darwin":
        return ("lib" + name if not name.startswith("lib") else name) + ".dylib"
    else:
        return ("lib" + name if not name.startswith("lib") else name) + ".so"


def load_library(name):
    resolved = _normalize_libname(name)
    p = pathlib.Path(resolved)

    if p.is_absolute() and p.exists():
        return str(p)

    search = [
        pathlib.Path.cwd() / resolved,
        _CLANGLIB_DIR / resolved,
    ]

    if sys.platform == "win32":
        for d in os.environ.get("PATH", "").split(os.pathsep):
            search.append(pathlib.Path(d) / resolved)
    else:
        for d in os.environ.get("LD_LIBRARY_PATH", "").split(os.pathsep):
            search.append(pathlib.Path(d) / resolved)
        search += [
            pathlib.Path("/usr/lib") / resolved,
            pathlib.Path("/usr/local/lib") / resolved,
        ]

    for candidate in search:
        if candidate.exists():
            return str(candidate)

    return resolved