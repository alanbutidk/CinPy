#include "loader.h"
#include "cpyc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define PY_VERSION_MAJOR 3
#define PY_VERSION_MINOR 12

static int read_u8(FILE *f, uint8_t *out)
{
    return fread(out, 1, 1, f) == 1 ? 0 : -1;
}

static int read_u16(FILE *f, uint16_t *out)
{
    uint8_t buf[2];
    if (fread(buf, 1, 2, f) != 2)
        return -1;
    *out = (uint16_t)(buf[0] | (buf[1] << 8));
    return 0;
}

static char *read_str(FILE *f)
{
    char buf[4096];
    size_t i = 0;
    int c;
    while (i < sizeof(buf) - 1) {
        c = fgetc(f);
        if (c == EOF)
            return NULL;
        buf[i++] = (char)c;
        if (c == '\0')
            break;
    }
    buf[i] = '\0';
    return strdup(buf);
}

CpycStatus cinpy_load_cpyc(const char *path, CinPyIR **out)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return CPYC_ERR_OPEN;

    char magic[CPYC_MAGIC_LEN];
    if (fread(magic, 1, CPYC_MAGIC_LEN, f) != CPYC_MAGIC_LEN ||
        memcmp(magic, CPYC_MAGIC, CPYC_MAGIC_LEN) != 0) {
        fclose(f);
        return CPYC_ERR_MAGIC;
    }

    uint16_t py_maj, py_min, cp_maj, cp_min;
    if (read_u16(f, &py_maj) || read_u16(f, &py_min) ||
        read_u16(f, &cp_maj) || read_u16(f, &cp_min)) {
        fclose(f);
        return CPYC_ERR_CORRUPT;
    }

    if (py_maj != PY_VERSION_MAJOR || py_min != PY_VERSION_MINOR) {
        fclose(f);
        return CPYC_ERR_PY_VERSION;
    }

    if (cp_maj != CINPY_MAJOR) {
        fclose(f);
        return CPYC_ERR_CINPY_VERSION;
    }

    char *hdr_name = read_str(f);
    if (!hdr_name) {
        fclose(f);
        return CPYC_ERR_CORRUPT;
    }

    CinPyIR *ir = cinpy_ir_new(hdr_name);
    free(hdr_name);
    if (!ir) {
        fclose(f);
        return CPYC_ERR_OOM;
    }

    while (1) {
        uint8_t peek;
        if (read_u8(f, &peek)) {
            cinpy_ir_free(ir);
            fclose(f);
            return CPYC_ERR_CORRUPT;
        }

        if (peek == CPYC_SENTINEL)
            break;

        ungetc(peek, f);

        char *fn_name = read_str(f);
        if (!fn_name) {
            cinpy_ir_free(ir);
            fclose(f);
            return CPYC_ERR_CORRUPT;
        }

        uint8_t ret_type, param_count;
        if (read_u8(f, &ret_type) || read_u8(f, &param_count)) {
            free(fn_name);
            cinpy_ir_free(ir);
            fclose(f);
            return CPYC_ERR_CORRUPT;
        }

        CinPyFnRecord rec = {0};
        rec.name        = fn_name;
        rec.ret         = (CinPyType)ret_type;
        rec.param_count = param_count;
        rec.params      = calloc(param_count, sizeof(CinPyParam));

        if (param_count > 0 && !rec.params) {
            free(fn_name);
            cinpy_ir_free(ir);
            fclose(f);
            return CPYC_ERR_OOM;
        }

        for (uint8_t i = 0; i < param_count; i++) {
            rec.params[i].name = read_str(f);
            uint8_t pt;
            if (!rec.params[i].name || read_u8(f, &pt)) {
                for (uint8_t k = 0; k <= i; k++)
                    free(rec.params[k].name);
                free(rec.params);
                free(fn_name);
                cinpy_ir_free(ir);
                fclose(f);
                return CPYC_ERR_CORRUPT;
            }
            rec.params[i].type = (CinPyType)pt;
        }

        cinpy_ir_push(ir, &rec);
    }

    fclose(f);
    *out = ir;
    return CPYC_OK;
}
