#include "call.h"
#include "cache.h"
#include "marshal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
static void *resolve_sym(const char *name)
{
    HMODULE h = GetModuleHandleA(NULL);
    void *sym = (void *)GetProcAddress(h, name);
    if (!sym) {
        HMODULE libs[] = {
            GetModuleHandleA("msvcrt.dll"),
            GetModuleHandleA("ucrtbase.dll"),
            NULL
        };
        for (int i = 0; libs[i]; i++) {
            sym = (void *)GetProcAddress(libs[i], name);
            if (sym) break;
        }
    }
    return sym;
}
#else
#include <dlfcn.h>
static void *resolve_sym(const char *name)
{
    return dlsym(RTLD_DEFAULT, name);
}
#endif

PyObject *cinpy_call(const char *fn_name, const CinPyFnRecord *rec,
                     PyObject *py_args, Py_ssize_t n_given)
{
    CacheEntry *cached = cinpy_cache_get(fn_name);
    ffi_cif cif;
    void *fn_ptr = NULL;

    if (cached) {
        cif = cached->cif;
        fn_ptr = cached->fn_ptr;
    } else {
        fn_ptr = resolve_sym(fn_name);
        if (!fn_ptr) {
            PyErr_Format(PyExc_RuntimeError,
                "CinPy: could not resolve symbol '%s'", fn_name);
            return NULL;
        }

        uint8_t np = rec->param_count;
        ffi_type **arg_types = malloc((np ? np : 1) * sizeof(ffi_type *));
        if (!arg_types) {
            PyErr_NoMemory();
            return NULL;
        }
        for (uint8_t i = 0; i < np; i++)
            arg_types[i] = cinpy_type_to_ffi(rec->params[i].type);

        ffi_type *ret_type = cinpy_type_to_ffi(rec->ret);

        if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, np, ret_type, arg_types) != FFI_OK) {
            free(arg_types);
            PyErr_SetString(PyExc_RuntimeError, "CinPy: ffi_prep_cif failed");
            return NULL;
        }

        cinpy_cache_put(fn_name, fn_ptr, &cif, arg_types);
    }

    uint8_t n = rec->param_count;
    if (n_given != (Py_ssize_t)n) {
        PyErr_Format(PyExc_TypeError,
            "%s() takes %d argument(s), got %zd", fn_name, n, n_given);
        return NULL;
    }

    CinPyArg *args = malloc((n ? n : 1) * sizeof(CinPyArg));
    void **aptrs = malloc((n ? n : 1) * sizeof(void *));
    if (!args || !aptrs) {
        free(args);
        free(aptrs);
        PyErr_NoMemory();
        return NULL;
    }

    for (uint8_t i = 0; i < n; i++) {
        PyObject *item = PyTuple_GetItem(py_args, i);
        if (cinpy_py_to_ffi(item, rec->params[i].type, &args[i]) < 0) {
            free(args);
            free(aptrs);
            return NULL;
        }
        aptrs[i] = args[i].ptr;
    }

    CinPyRetVal ret_val;
    memset(&ret_val, 0, sizeof(ret_val));
    ffi_call(&cif, FFI_FN(fn_ptr), &ret_val, n > 0 ? aptrs : NULL);

    free(args);
    free(aptrs);

    return cinpy_ffi_to_py(&ret_val, rec->ret);
}