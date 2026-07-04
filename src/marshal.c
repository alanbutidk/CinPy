#include "marshal.h"
#include <string.h>

ffi_type *cinpy_type_to_ffi(CinPyType t)
{
    switch (t) {
        case CINPY_VOID:     return &ffi_type_void;
        case CINPY_BOOL:     return &ffi_type_uint8;
        case CINPY_INT:      return &ffi_type_sint;
        case CINPY_UINT:     return &ffi_type_uint;
        case CINPY_LONG:     return &ffi_type_slong;
        case CINPY_ULONG:    return &ffi_type_ulong;
        case CINPY_LONGLONG: return &ffi_type_sint64;
        case CINPY_FLOAT:    return &ffi_type_float;
        case CINPY_DOUBLE:   return &ffi_type_double;
        case CINPY_CHARPTR:
        case CINPY_PTR:
        case CINPY_SIZE_T:   return &ffi_type_pointer;
        default:             return &ffi_type_pointer;
    }
}

int cinpy_py_to_ffi(PyObject *obj, CinPyType t, CinPyArg *out)
{
    memset(out, 0, sizeof(*out));
    switch (t) {
        case CINPY_BOOL:
            out->as_uint = (unsigned int)PyObject_IsTrue(obj);
            out->ptr = &out->as_uint;
            return 0;
        case CINPY_INT:
            out->as_int = (int)PyLong_AsLong(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_int;
            return 0;
        case CINPY_UINT:
            out->as_uint = (unsigned int)PyLong_AsUnsignedLong(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_uint;
            return 0;
        case CINPY_LONG:
            out->as_long = PyLong_AsLong(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_long;
            return 0;
        case CINPY_ULONG:
            out->as_ulong = PyLong_AsUnsignedLong(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_ulong;
            return 0;
        case CINPY_LONGLONG:
            out->as_ll = PyLong_AsLongLong(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_ll;
            return 0;
        case CINPY_FLOAT:
            out->as_float = (float)PyFloat_AsDouble(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_float;
            return 0;
        case CINPY_DOUBLE:
            out->as_double = PyFloat_AsDouble(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_double;
            return 0;
        case CINPY_CHARPTR:
        case CINPY_PTR:
            if (PyUnicode_Check(obj)) {
                out->as_ptr = (void *)PyUnicode_AsUTF8(obj);
                if (!out->as_ptr) return -1;
            } else if (PyLong_Check(obj)) {
                out->as_ptr = PyLong_AsVoidPtr(obj);
                if (PyErr_Occurred()) return -1;
            } else if (obj == Py_None) {
                out->as_ptr = NULL;
            } else {
                PyErr_SetString(PyExc_TypeError,
                    "expected str, int, or None for pointer arg");
                return -1;
            }
            out->ptr = &out->as_ptr;
            return 0;
        case CINPY_SIZE_T:
            out->as_ptr = PyLong_AsVoidPtr(obj);
            if (PyErr_Occurred()) return -1;
            out->ptr = &out->as_ptr;
            return 0;
        default:
            PyErr_SetString(PyExc_TypeError, "CinPy: unsupported argument type");
            return -1;
    }
}

PyObject *cinpy_ffi_to_py(CinPyRetVal *ret, CinPyType t)
{
    switch (t) {
        case CINPY_VOID:
            Py_RETURN_NONE;
        case CINPY_BOOL:
            return PyBool_FromLong((long)ret->as_uint8);
        case CINPY_INT:
            return PyLong_FromLong((long)ret->as_int);
        case CINPY_UINT:
            return PyLong_FromUnsignedLong((unsigned long)ret->as_uint);
        case CINPY_LONG:
            return PyLong_FromLong(ret->as_long);
        case CINPY_ULONG:
            return PyLong_FromUnsignedLong(ret->as_ulong);
        case CINPY_LONGLONG:
            return PyLong_FromLongLong(ret->as_ll);
        case CINPY_FLOAT:
            return PyFloat_FromDouble((double)ret->as_float);
        case CINPY_DOUBLE:
            return PyFloat_FromDouble(ret->as_double);
        case CINPY_CHARPTR: {
            const char *s = ret->as_ptr;
            if (!s) Py_RETURN_NONE;
            return PyUnicode_FromString(s);
        }
        case CINPY_PTR:
        case CINPY_SIZE_T:
            return PyLong_FromVoidPtr(ret->as_ptr);
        default:
            Py_RETURN_NONE;
    }
}