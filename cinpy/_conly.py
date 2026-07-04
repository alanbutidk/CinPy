import ctypes
import struct as _struct
from cinpy._marshal_hints import CINPY_TYPE_MAP


class CVar:
    __slots__ = ("_name", "_type", "_val", "_ctype")

    def __init__(self, name, ctype, initial=None):
        self._name  = name
        self._type  = ctype
        self._ctype = ctype
        self._val   = ctype(initial if initial is not None else ctype._type_())

    def value(self, v=None):
        if v is None:
            return self._val.value
        self._val = self._ctype(v)
        return self

    def __cinpy_unwrap__(self):
        return self._val.value

    def __repr__(self):
        return f"<COnly.{self._type.__name__} '{self._name}' = {self._val.value!r}>"


class CCharPtrVar:
    __slots__ = ("_name", "_val")

    def __init__(self, name, initial=None):
        self._name = name
        self._val  = (initial or "").encode() if isinstance(initial, str) else (initial or b"")

    def value(self, v=None):
        if v is None:
            return self._val.decode(errors="replace")
        self._val = v.encode() if isinstance(v, str) else v
        return self

    def __cinpy_unwrap__(self):
        return self._val.decode(errors="replace")

    def __repr__(self):
        return f"<COnly.charptr '{self._name}' = {self._val!r}>"


class CVoidPtrVar:
    __slots__ = ("_name", "_val")

    def __init__(self, name, initial=None):
        self._name = name
        self._val  = initial

    def value(self, v=None):
        if v is None:
            return self._val
        self._val = v
        return self

    def __cinpy_unwrap__(self):
        return self._val

    def __repr__(self):
        return f"<COnly.voidptr '{self._name}' = {self._val!r}>"


class CStructField:
    def __init__(self, name, field_type):
        self._name  = name
        self._inner = field_type(name)

    def value(self, v=None):
        return self._inner.value(v)

    def __cinpy_unwrap__(self):
        return self._inner.__cinpy_unwrap__()

    def __repr__(self):
        return repr(self._inner)


class CStructInstance:
    def __init__(self, tag, fields, name):
        self.__dict__["_tag"]    = tag
        self.__dict__["_name"]   = name
        self.__dict__["_fields"] = {}
        for fname, ftype in fields:
            self.__dict__["_fields"][fname] = CStructField(fname, ftype)

    def __getattr__(self, name):
        fields = self.__dict__["_fields"]
        if name in fields:
            return fields[name]
        raise AttributeError(f"struct '{self.__dict__['_tag']}' has no field '{name}'")

    def __setattr__(self, name, val):
        fields = self.__dict__["_fields"]
        if name in fields:
            fields[name].value(val)
        else:
            self.__dict__[name] = val

    def __repr__(self):
        tag    = self.__dict__["_tag"]
        name   = self.__dict__["_name"]
        fields = self.__dict__["_fields"]
        inner  = ", ".join(f"{k}={v.value()!r}" for k, v in fields.items())
        return f"<COnly.struct '{tag}' '{name}' {{{inner}}}>"


class CStructType:
    def __init__(self, tag, fields):
        self._tag    = tag
        self._fields = fields

    def __call__(self, name):
        return CStructInstance(self._tag, self._fields, name)

    def __repr__(self):
        return f"<COnly.struct type '{self._tag}'>"


class CEnumInstance:
    def __init__(self, tag, members, name, initial=None):
        self._tag     = tag
        self._members = members
        self._rmap    = {v: k for k, v in members.items()}
        self._name    = name
        if initial is not None:
            if isinstance(initial, str):
                self._val = members[initial]
            else:
                self._val = int(initial)
        else:
            self._val = next(iter(members.values()))

    def value(self, v=None):
        if v is None:
            return self._rmap.get(self._val, self._val)
        if isinstance(v, str):
            self._val = self._members[v]
        else:
            self._val = int(v)
        return self

    def int_value(self):
        return self._val

    def __cinpy_unwrap__(self):
        return self._val

    def __repr__(self):
        name = self._rmap.get(self._val, self._val)
        return f"<COnly.enum '{self._tag}' '{self._name}' = {name!r}>"


class CEnumType:
    def __init__(self, tag, members):
        self._tag     = tag
        self._members = members

    def __call__(self, name, initial=None):
        return CEnumInstance(self._tag, self._members, name, initial)

    def __repr__(self):
        return f"<COnly.enum type '{self._tag}'>"


class CArrayVar:
    def __init__(self, name, elem_type, size, initial=None):
        self._name      = name
        self._elem_type = elem_type
        self._size      = size
        self._items     = [elem_type(f"{name}[{i}]") for i in range(size)]
        if initial:
            for i, v in enumerate(initial[:size]):
                self._items[i].value(v)

    def __getitem__(self, idx):
        return self._items[idx]

    def __setitem__(self, idx, val):
        self._items[idx].value(val)

    def value(self):
        return [item.value() for item in self._items]

    def __cinpy_unwrap__(self):
        return self.value()

    def __repr__(self):
        return f"<COnly.array '{self._name}' {self.value()!r}>"


def _make_scalar(ctypes_type, type_name):
    class ScalarType:
        _ct = ctypes_type
        __name__ = type_name

        def __new__(cls, name, initial=None):
            obj = object.__new__(cls)
            obj._name = name
            obj._ct   = ctypes_type
            if initial is not None:
                v = initial if not isinstance(initial, str) else type(ctypes_type._type_())(initial)
                obj._val = ctypes_type(v)
            else:
                obj._val = ctypes_type(0)
            return obj

        def value(self, v=None):
            if v is None:
                return self._val.value
            self._val = self._ct(v)
            return self

        def __cinpy_unwrap__(self):
            return self._val.value

        def __repr__(self):
            return f"<COnly.{type_name} '{self._name}' = {self._val.value!r}>"

    ScalarType.__name__     = type_name
    ScalarType.__qualname__ = type_name
    return ScalarType


class _COnly:
    int       = _make_scalar(ctypes.c_int,       "int")
    uint      = _make_scalar(ctypes.c_uint,      "uint")
    long      = _make_scalar(ctypes.c_long,      "long")
    ulong     = _make_scalar(ctypes.c_ulong,     "ulong")
    longlong  = _make_scalar(ctypes.c_longlong,  "longlong")
    ulonglong = _make_scalar(ctypes.c_ulonglong, "ulonglong")
    float     = _make_scalar(ctypes.c_float,     "float")
    double    = _make_scalar(ctypes.c_double,    "double")
    char      = _make_scalar(ctypes.c_char,      "char")
    size_t    = _make_scalar(ctypes.c_size_t,    "size_t")
    bool      = _make_scalar(ctypes.c_bool,      "bool")

    charptr   = CCharPtrVar
    voidptr   = CVoidPtrVar

    @staticmethod
    def intptr(name, initial=None):
        v = ctypes.c_int(initial if initial is not None else 0)
        ref = ctypes.pointer(v)
        obj = CVoidPtrVar(name, ref)
        obj.__cinpy_unwrap__ = lambda: ctypes.addressof(v)
        return obj

    @staticmethod
    def floatptr(name, initial=None):
        v = ctypes.c_float(initial if initial is not None else 0.0)
        ref = ctypes.pointer(v)
        obj = CVoidPtrVar(name, ref)
        obj.__cinpy_unwrap__ = lambda: ctypes.addressof(v)
        return obj

    @staticmethod
    def doubleptr(name, initial=None):
        v = ctypes.c_double(initial if initial is not None else 0.0)
        ref = ctypes.pointer(v)
        obj = CVoidPtrVar(name, ref)
        obj.__cinpy_unwrap__ = lambda: ctypes.addressof(v)
        return obj

    @staticmethod
    def struct(tag, fields):
        return CStructType(tag, fields)

    @staticmethod
    def enum(tag, members):
        return CEnumType(tag, members)

    @staticmethod
    def array(elem_type, size):
        def factory(name, initial=None):
            return CArrayVar(name, elem_type, size, initial)
        factory.__name__ = f"array({elem_type.__name__}, {size})"
        return factory


COnly = _COnly()