#ifndef CALL_H
#define CALL_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "ir.h"

PyObject *cinpy_call(const char *fn_name, const CinPyFnRecord *rec,
                     PyObject *py_args, Py_ssize_t n_given);

#endif
