#include "emitter.h"
#include "cpyc.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define PY_VERSION_MAJOR 3
#define PY_VERSION_MINOR 12

static int write_str(FILE *f, const char *s)
{
    if (!s) s = "";
    size_t len = strlen(s) + 1;
    return fwrite(s, 1, len, f) == len ? 0 : -1;
}

static int write_u8(FILE *f, uint8_t v)
{
    return fwrite(&v, 1, 1, f) == 1 ? 0 : -1;
}

static int write_u16(FILE *f, uint16_t v)
{
    uint8_t buf[2] = { (uint8_t)(v & 0xFF), (uint8_t)(v >> 8) };
    return fwrite(buf, 1, 2, f) == 2 ? 0 : -1;
}

CpycStatus cinpy_emit_cpyc(const CinPyIR *ir, const char *out_path)
{
    FILE *f = fopen(out_path, "wb");
    if (!f)
        return CPYC_ERR_OPEN;

    if (fwrite(CPYC_MAGIC, 1, CPYC_MAGIC_LEN, f) != CPYC_MAGIC_LEN)
        goto err;

    if (write_u16(f, PY_VERSION_MAJOR)) goto err;
    if (write_u16(f, PY_VERSION_MINOR)) goto err;
    if (write_u16(f, CINPY_MAJOR))      goto err;
    if (write_u16(f, CINPY_MINOR))      goto err;

    if (write_str(f, ir->header_name))  goto err;

    for (size_t i = 0; i < ir->fn_count; i++) {
        const CinPyFnRecord *fn = &ir->fns[i];
        if (write_str(f, fn->name))           goto err;
        if (write_u8(f, (uint8_t)fn->ret))    goto err;
        if (write_u8(f, fn->param_count))     goto err;
        for (uint8_t j = 0; j < fn->param_count; j++) {
            if (write_str(f, fn->params[j].name))         goto err;
            if (write_u8(f, (uint8_t)fn->params[j].type)) goto err;
        }
    }

    uint8_t sentinel = CPYC_SENTINEL;
    if (fwrite(&sentinel, 1, 1, f) != 1) goto err;

    fclose(f);
    return CPYC_OK;

err:
    fclose(f);
    return CPYC_ERR_CORRUPT;
}
