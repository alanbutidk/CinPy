#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>
#include <string.h>

#include "clang_bridge.h"
#include "parser.h"
#include "emitter.h"
#include "loader.h"
#include "cache.h"
#include "call.h"
#include "ir.h"
#include "cpyc.h"

static PyObject *CinPyError;

static void ir_capsule_destructor(PyObject *cap)
{
    CinPyIR *ir = (CinPyIR *)PyCapsule_GetPointer(cap, "CinPyIR");
    if (ir)
        cinpy_ir_free(ir);
}

static CinPyIR *ir_from_capsule(PyObject *cap)
{
    return (CinPyIR *)PyCapsule_GetPointer(cap, "CinPyIR");
}

static PyObject *py_init_clang(PyObject *self, PyObject *args)
{
    (void)self;
    const char *dll_path;
    if (!PyArg_ParseTuple(args, "s", &dll_path))
        return NULL;
    if (clang_bridge_init(&g_clang, dll_path) < 0) {
        PyErr_Format(CinPyError, "failed to load libclang from '%s'", dll_path);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *py_parse_header(PyObject *self, PyObject *args)
{
    (void)self;
    const char *header;
    const char *inc_path = NULL;
    if (!PyArg_ParseTuple(args, "s|z", &header, &inc_path))
        return NULL;
    CinPyIR *ir = cinpy_parse_header(header, NULL, 0, inc_path);
    if (!ir) {
        PyErr_Format(CinPyError, "failed to parse header '%s'", header);
        return NULL;
    }
    return PyCapsule_New(ir, "CinPyIR", ir_capsule_destructor);
}

static PyObject *py_gen_cpyc(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject   *cap;
    const char *out_path;
    if (!PyArg_ParseTuple(args, "Os", &cap, &out_path))
        return NULL;
    CinPyIR *ir = ir_from_capsule(cap);
    if (!ir) return NULL;
    CpycStatus st = cinpy_emit_cpyc(ir, out_path);
    if (st != CPYC_OK) {
        PyErr_Format(CinPyError, "GenCPyC failed with status %d", (int)st);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *py_load_cpyc(PyObject *self, PyObject *args)
{
    (void)self;
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;
    CinPyIR *ir = NULL;
    CpycStatus st = cinpy_load_cpyc(path, &ir);
    if (st != CPYC_OK) {
        PyErr_Format(CinPyError, "LoadCPyC failed: status %d for '%s'", (int)st, path);
        return NULL;
    }
    return PyCapsule_New(ir, "CinPyIR", ir_capsule_destructor);
}

static PyObject *py_ir_fn_list(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject *cap;
    if (!PyArg_ParseTuple(args, "O", &cap))
        return NULL;
    CinPyIR *ir = ir_from_capsule(cap);
    if (!ir) return NULL;

    PyObject *list = PyList_New((Py_ssize_t)ir->fn_count);
    if (!list) return NULL;
    for (size_t i = 0; i < ir->fn_count; i++) {
        PyObject *name = PyUnicode_FromString(ir->fns[i].name);
        if (!name) { Py_DECREF(list); return NULL; }
        PyList_SET_ITEM(list, (Py_ssize_t)i, name);
    }
    return list;
}

static PyObject *py_call_fn(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject   *cap;
    const char *fn_name;
    PyObject   *fn_args;

    if (!PyArg_ParseTuple(args, "OsO", &cap, &fn_name, &fn_args))
        return NULL;

    CinPyIR *ir = ir_from_capsule(cap);
    if (!ir) return NULL;

    CinPyFnRecord *rec = NULL;
    for (size_t i = 0; i < ir->fn_count; i++) {
        if (strcmp(ir->fns[i].name, fn_name) == 0) {
            rec = &ir->fns[i];
            break;
        }
    }
    if (!rec) {
        PyErr_Format(PyExc_AttributeError, "CinPy: no function '%s' in IR", fn_name);
        return NULL;
    }

    if (!PyTuple_Check(fn_args)) {
        PyErr_SetString(PyExc_TypeError, "CinPy: fn_args must be a tuple");
        return NULL;
    }

    return cinpy_call(fn_name, rec, fn_args, PyTuple_GET_SIZE(fn_args));
}

static void fn_record_capsule_destructor(PyObject *cap)
{
    PyObject *ir_cap = (PyObject *)PyCapsule_GetContext(cap);
    Py_XDECREF(ir_cap);
}

static PyObject *py_resolve_fn_record(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject   *ir_cap;
    const char *fn_name;

    if (!PyArg_ParseTuple(args, "Os", &ir_cap, &fn_name))
        return NULL;

    CinPyIR *ir = ir_from_capsule(ir_cap);
    if (!ir) return NULL;

    for (size_t i = 0; i < ir->fn_count; i++) {
        if (strcmp(ir->fns[i].name, fn_name) == 0) {
            PyObject *rec_cap = PyCapsule_New(&ir->fns[i], "CinPyFnRecord",
                                              fn_record_capsule_destructor);
            if (!rec_cap) return NULL;
            Py_INCREF(ir_cap);
            if (PyCapsule_SetContext(rec_cap, ir_cap) < 0) {
                Py_DECREF(ir_cap);
                Py_DECREF(rec_cap);
                return NULL;
            }
            return rec_cap;
        }
    }

    PyErr_Format(PyExc_AttributeError,
        "CinPy: no function '%s' in IR", fn_name);
    return NULL;
}

static PyObject *py_internal_function(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject *rec_cap;
    PyObject *fn_args;

    if (!PyArg_ParseTuple(args, "OO", &rec_cap, &fn_args))
        return NULL;

    CinPyFnRecord *rec = (CinPyFnRecord *)PyCapsule_GetPointer(
        rec_cap, "CinPyFnRecord");
    if (!rec) return NULL;

    if (!PyTuple_Check(fn_args)) {
        PyErr_SetString(PyExc_TypeError, "CinPy: fn_args must be a tuple");
        return NULL;
    }

    return cinpy_call(rec->name, rec, fn_args, PyTuple_GET_SIZE(fn_args));
}

static PyObject *py_cache_evict(PyObject *self, PyObject *args)
{
    (void)self; (void)args;
    cinpy_cache_evict_all();
    Py_RETURN_NONE;
}

static PyObject *py_config(PyObject *self, PyObject *args)
{
    (void)self;
    int cap = 0, timeout = 0;
    if (!PyArg_ParseTuple(args, "|ii", &cap, &timeout))
        return NULL;
    cinpy_cache_config((size_t)cap, timeout);
    Py_RETURN_NONE;
}

static PyMethodDef cinpy_methods[] = {
    {"_init_clang",   py_init_clang,   METH_VARARGS, NULL},
    {"_parse_header", py_parse_header, METH_VARARGS, NULL},
    {"_gen_cpyc",     py_gen_cpyc,     METH_VARARGS, NULL},
    {"_load_cpyc",    py_load_cpyc,    METH_VARARGS, NULL},
    {"_ir_fn_list",   py_ir_fn_list,   METH_VARARGS, NULL},
    {"_call_fn",           py_call_fn,           METH_VARARGS, NULL},
    {"_resolve_fn_record", py_resolve_fn_record,  METH_VARARGS, NULL},
    {"_InternalFunction",  py_internal_function,  METH_VARARGS, NULL},
    {"_cache_evict",       py_cache_evict,        METH_NOARGS,  NULL},
    {"_config",       py_config,       METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef cinpy_module = {
    PyModuleDef_HEAD_INIT,
    "cinpy_core",
    NULL,
    -1,
    cinpy_methods
};

PyMODINIT_FUNC PyInit_cinpy_core(void)
{
    if (cinpy_cache_init() < 0)
        return NULL;

    PyObject *m = PyModule_Create(&cinpy_module);
    if (!m)
        return NULL;

    CinPyError = PyErr_NewException("cinpy_core.CinPyError", NULL, NULL);
    if (!CinPyError) { Py_DECREF(m); return NULL; }
    PyModule_AddObject(m, "CinPyError", CinPyError);

    return m;
}