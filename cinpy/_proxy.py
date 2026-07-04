from cinpy_core import _InternalFunction, _resolve_fn_record, _ir_fn_list, CinPyError
from cinpy._fmt import rewrite_fmt_args


class FnProxy:
    def __init__(self, rec_cap, name):
        self._rec_cap = rec_cap
        self._name    = name

    def __call__(self, *args):
        args = rewrite_fmt_args(self._name, args)
        return _InternalFunction(self._rec_cap, args)

    def __repr__(self):
        return f"<CinPy fn '{self._name}'>"


class HeaderProxy:
    def __init__(self, ir_cap):
        self.__dict__['_ir_cap']    = ir_cap
        self.__dict__['_fn_names']  = set(_ir_fn_list(ir_cap))
        self.__dict__['_rec_cache'] = {}

    def __getattr__(self, name):
        if name not in self._fn_names:
            raise AttributeError(f"CinPy: no function '{name}' in loaded header")
        if name not in self._rec_cache:
            self._rec_cache[name] = _resolve_fn_record(self._ir_cap, name)
        return FnProxy(self._rec_cache[name], name)

    def functions(self):
        return sorted(self._fn_names)

    def __repr__(self):
        return f"<CinPy header with {len(self._fn_names)} function(s)>"