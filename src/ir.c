#include "ir.h"
#include "clang_bridge.h"
#include <stdlib.h>
#include <string.h>

#define IR_INIT_CAP 32

CinPyIR *cinpy_ir_new(const char *header_name)
{
    CinPyIR *ir = calloc(1, sizeof(*ir));
    if (!ir)
        return NULL;

    ir->header_name = strdup(header_name);
    ir->fns         = malloc(IR_INIT_CAP * sizeof(CinPyFnRecord));
    ir->fn_count    = 0;
    ir->fn_cap      = IR_INIT_CAP;

    if (!ir->header_name || !ir->fns) {
        cinpy_ir_free(ir);
        return NULL;
    }
    return ir;
}

int cinpy_ir_push(CinPyIR *ir, CinPyFnRecord *fn)
{
    if (ir->fn_count == ir->fn_cap) {
        size_t new_cap = ir->fn_cap * 2;
        CinPyFnRecord *tmp = realloc(ir->fns, new_cap * sizeof(CinPyFnRecord));
        if (!tmp)
            return -1;
        ir->fns    = tmp;
        ir->fn_cap = new_cap;
    }
    ir->fns[ir->fn_count++] = *fn;
    return 0;
}

void cinpy_ir_free(CinPyIR *ir)
{
    if (!ir)
        return;
    for (size_t i = 0; i < ir->fn_count; i++) {
        free(ir->fns[i].name);
        for (uint8_t j = 0; j < ir->fns[i].param_count; j++)
            free(ir->fns[i].params[j].name);
        free(ir->fns[i].params);
    }
    free(ir->fns);
    free(ir->header_name);
    free(ir);
}

CinPyType cinpy_map_type_kind(int kind)
{
    switch (kind) {
        case 2:  return CINPY_VOID;
        case 3:  return CINPY_BOOL;
        case 4:
        case 5:
        case 6:  return CINPY_INT;
        case 7:
        case 8:  return CINPY_UINT;
        case 9:
        case 10: return CINPY_LONG;
        case 11:
        case 12: return CINPY_ULONG;
        case 13:
        case 14: return CINPY_LONGLONG;
        case 21: return CINPY_FLOAT;
        case 22: return CINPY_DOUBLE;
        case 101: return CINPY_PTR;
        default:  return CINPY_UNKNOWN;
    }
}
