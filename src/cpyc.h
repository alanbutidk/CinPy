#ifndef CPYC_H
#define CPYC_H

#include <stdint.h>

#define CPYC_MAGIC        "CINPYSTORED"
#define CPYC_MAGIC_LEN    12
#define CPYC_SENTINEL     0xFF

#define CINPY_MAJOR       1
#define CINPY_MINOR       0

typedef enum {
    CINPY_VOID     = 0,
    CINPY_INT      = 1,
    CINPY_UINT     = 2,
    CINPY_LONG     = 3,
    CINPY_ULONG    = 4,
    CINPY_LONGLONG = 5,
    CINPY_FLOAT    = 6,
    CINPY_DOUBLE   = 7,
    CINPY_CHARPTR  = 8,
    CINPY_PTR      = 9,
    CINPY_SIZE_T   = 10,
    CINPY_BOOL     = 11,
    CINPY_UNKNOWN  = 255
} CinPyType;

typedef enum {
    CPYC_OK               = 0,
    CPYC_ERR_OPEN         = 1,
    CPYC_ERR_MAGIC        = 2,
    CPYC_ERR_PY_VERSION   = 3,
    CPYC_ERR_CINPY_VERSION= 4,
    CPYC_ERR_CORRUPT      = 5,
    CPYC_ERR_OOM          = 6
} CpycStatus;

#endif
