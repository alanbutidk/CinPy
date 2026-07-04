#ifndef MARSHAL_H
#define MARSHAL_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <ffi.h>
#include <stdint.h>
#include "cpyc.h"

typedef struct {
    void *ptr;
    union {
        int           as_int;
        unsigned int  as_uint;
        long          as_long;
        unsigned long as_ulong;
        long long     as_ll;
        float         as_float;
        double        as_double;
        void         *as_ptr;
    };
} CinPyArg;

typedef union {
    uint8_t       as_uint8;
    int           as_int;
    unsigned int  as_uint;
    long          as_long;
    unsigned long as_ulong;
    long long     as_ll;
    float         as_float;
    double        as_double;
    void         *as_ptr;
    uint64_t      as_u64;
} CinPyRetVal;

ffi_type  *cinpy_type_to_ffi(CinPyType t);
int        cinpy_py_to_ffi(PyObject *obj, CinPyType t, CinPyArg *out);
PyObject  *cinpy_ffi_to_py(CinPyRetVal *ret, CinPyType t);

#endif