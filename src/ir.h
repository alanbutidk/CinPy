#ifndef IR_H
#define IR_H

#include <stddef.h>
#include <stdint.h>
#include "cpyc.h"

typedef struct {
    char       *name;
    CinPyType   type;
} CinPyParam;

typedef struct {
    char       *name;
    CinPyType   ret;
    uint8_t     param_count;
    CinPyParam *params;
} CinPyFnRecord;

typedef struct {
    char          *header_name;
    CinPyFnRecord *fns;
    size_t         fn_count;
    size_t         fn_cap;
} CinPyIR;

CinPyIR *cinpy_ir_new(const char *header_name);
int      cinpy_ir_push(CinPyIR *ir, CinPyFnRecord *fn);
void     cinpy_ir_free(CinPyIR *ir);

CinPyType cinpy_map_type_kind(int kind);

#endif
