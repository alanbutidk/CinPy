import ctypes
import sys
from cinpy._conly import COnly

if sys.platform == "win32":
    import ctypes.wintypes
    _kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)


def _get_proc(handle, name):
    if sys.platform == "win32":
        addr = _kernel32.GetProcAddress(handle, name.encode())
        return addr
    else:
        try:
            return ctypes.cast(getattr(handle, name), ctypes.c_void_p).value
        except AttributeError:
            return None


def _unwrap(arg):
    if hasattr(arg, "__cinpy_unwrap__"):
        return arg.__cinpy_unwrap__()
    return arg


class LibFnProxy:
    def __init__(self, lib_handle, name, is_win):
        self._handle = lib_handle
        self._name   = name
        self._is_win = is_win
        self._fn     = None

    def _resolve(self):
        if self._fn is not None:
            return self._fn
        if self._is_win:
            addr = _kernel32.GetProcAddress(self._handle, self._name.encode())
            if not addr:
                raise AttributeError(
                    f"CinPy: symbol '{self._name}' not found in library"
                )
            self._fn = ctypes.CFUNCTYPE(ctypes.c_int)(addr)
        else:
            self._fn = getattr(self._handle, self._name)
        return self._fn

    def __call__(self, *args):
        fn   = self._resolve()
        args = tuple(_unwrap(a) for a in args)
        return fn(*args)

    def __repr__(self):
        return f"<CinPy lib fn '{self._name}'>"


class LibProxy:
    def __init__(self, handle, path, is_win):
        self.__dict__["_handle"] = handle
        self.__dict__["_path"]   = path
        self.__dict__["_is_win"] = is_win
        self.__dict__["_cache"]  = {}

    def __getattr__(self, name):
        cache = self.__dict__["_cache"]
        if name not in cache:
            cache[name] = LibFnProxy(
                self.__dict__["_handle"],
                name,
                self.__dict__["_is_win"]
            )
        return cache[name]

    def __repr__(self):
        return f"<CinPy library '{self.__dict__['_path']}'>"