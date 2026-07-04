import os
import subprocess
import pathlib
import sys

_COMMON_INCLUDE_SUFFIXES = [
    "include",
    "usr/include",
    "usr/local/include",
]

_WIN_REGISTRY_ROOTS = [
    r"C:\Program Files\LLVM\include",
    r"C:\Program Files (x86)\LLVM\include",
]


def _try_compiler(header, compiler):
    try:
        result = subprocess.run(
            [compiler, "-print-file-name=" + header],
            capture_output=True, text=True, timeout=5
        )
        out = result.stdout.strip()
        if out and out != header and pathlib.Path(out).exists():
            return out
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    return None


def _try_compiler_include_dirs(header, compiler):
    try:
        result = subprocess.run(
            [compiler, "-E", "-x", "c", "-", "-v"],
            input="", capture_output=True, text=True, timeout=5
        )
        lines = result.stderr.splitlines()
        in_search = False
        for line in lines:
            if "#include <...> search starts here:" in line:
                in_search = True
                continue
            if "End of search list" in line:
                break
            if in_search:
                candidate = pathlib.Path(line.strip()) / header
                if candidate.exists():
                    return str(candidate)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    return None


def _try_path_search(header):
    path_dirs = os.environ.get("PATH", "").split(os.pathsep)
    for d in path_dirs:
        p = pathlib.Path(d)
        for suffix in _COMMON_INCLUDE_SUFFIXES:
            candidate = p.parent / suffix / header
            if candidate.exists():
                return str(candidate)
    return None


def _try_system_defaults(header):
    if sys.platform == "win32":
        for root in _WIN_REGISTRY_ROOTS:
            candidate = pathlib.Path(root) / header
            if candidate.exists():
                return str(candidate)
    else:
        for root in ["/usr/include", "/usr/local/include"]:
            candidate = pathlib.Path(root) / header
            if candidate.exists():
                return str(candidate)
    return None


def resolve_header(header, full_path=False, full_path_loc=None):
    if full_path:
        if not full_path_loc:
            raise ValueError(
                "FullPath=True requires FullPathLoc to be set"
            )
        p = pathlib.Path(full_path_loc)
        if not p.exists():
            raise FileNotFoundError(
                f"CinPy: header not found at '{full_path_loc}'"
            )
        return str(p)

    if pathlib.Path(header).is_absolute() and pathlib.Path(header).exists():
        return header

    for compiler in ("gcc", "clang", "cc", "x86_64-w64-mingw32-gcc"):
        result = _try_compiler(header, compiler)
        if result:
            return result

    for compiler in ("gcc", "clang", "cc"):
        result = _try_compiler_include_dirs(header, compiler)
        if result:
            return result

    result = _try_path_search(header)
    if result:
        return result

    result = _try_system_defaults(header)
    if result:
        return result

    raise FileNotFoundError(
        f"CinPy: could not locate '{header}'. "
        f"Use FullPath=True and FullPathLoc='path/to/{header}' to specify it manually."
    )