#include "parser.h"
#include "clang_bridge.h"
#include "ir.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    CinPyIR    *ir;
    const char *target_file;
} VisitCtx;

static int cursor_is_in_file(CXCursor cursor, const char *target)
{
    CXSourceLocation loc = g_clang.getCursorLocation(cursor);
    CXFile file;
    g_clang.getExpansionLocation(loc, &file, NULL, NULL, NULL);
    if (!file)
        return 0;
    CXString fname = g_clang.getFileName(file);
    const char *cstr = g_clang.getCString(fname);
    int match = cstr && strstr(cstr, target) != NULL;
    g_clang.disposeString(fname);
    return match;
}

static enum CXChildVisitResult visit_fn(CXCursor cursor, CXCursor parent, CXClientData data)
{
    (void)parent;
    VisitCtx *ctx = (VisitCtx *)data;

    if (g_clang.getCursorKind(cursor) != CXCursor_FunctionDecl)
        return CXChildVisit_Continue;

    if (!cursor_is_in_file(cursor, ctx->target_file))
        return CXChildVisit_Continue;

    CXString spelling = g_clang.getCursorSpelling(cursor);
    const char *name  = g_clang.getCString(spelling);

    CXType   fn_type  = g_clang.getCursorType(cursor);
    CXType   ret_cxt  = g_clang.getResultType(fn_type);
    int      n_params = g_clang.getNumArgTypes(fn_type);

    if (n_params < 0) {
        g_clang.disposeString(spelling);
        return CXChildVisit_Continue;
    }

    CinPyFnRecord rec = {0};
    rec.name        = strdup(name);
    rec.ret         = cinpy_map_type_kind(ret_cxt.kind);
    rec.param_count = (uint8_t)(n_params > 255 ? 255 : n_params);
    rec.params      = calloc(rec.param_count, sizeof(CinPyParam));

    if (!rec.name || (!rec.params && rec.param_count > 0)) {
        free(rec.name);
        free(rec.params);
        g_clang.disposeString(spelling);
        return CXChildVisit_Continue;
    }

    for (uint8_t i = 0; i < rec.param_count; i++) {
        CXType pt = g_clang.getArgType(fn_type, i);
        rec.params[i].type = cinpy_map_type_kind(pt.kind);

        CXString pts = g_clang.getTypeSpelling(pt);
        const char *pname = g_clang.getCString(pts);
        rec.params[i].name = pname ? strdup(pname) : strdup("arg");
        g_clang.disposeString(pts);
    }

    cinpy_ir_push(ctx->ir, &rec);
    g_clang.disposeString(spelling);
    return CXChildVisit_Continue;
}

CinPyIR *cinpy_parse_header(const char *header_path,
                             const char **extra_args, int n_args,
                             const char *clang_include_path)
{
    const char **args = NULL;
    int total = n_args + (clang_include_path ? 2 : 0);

    if (total > 0) {
        args = malloc(total * sizeof(char *));
        if (!args)
            return NULL;
        for (int i = 0; i < n_args; i++)
            args[i] = extra_args[i];
        if (clang_include_path) {
            args[n_args]     = "-I";
            args[n_args + 1] = clang_include_path;
        }
    }

    CXIndex idx = g_clang.createIndex(0, 0);
    CXTranslationUnit tu = g_clang.parseTranslationUnit(
        idx, header_path,
        args, total,
        NULL, 0,
        CXTranslationUnit_SkipFunctionBodies
    );
    free(args);

    if (!tu) {
        fprintf(stderr, "CinPy: clang failed to parse '%s'\n", header_path);
        g_clang.disposeIndex(idx);
        return NULL;
    }

    CinPyIR *ir = cinpy_ir_new(header_path);
    if (!ir) {
        g_clang.disposeTranslationUnit(tu);
        g_clang.disposeIndex(idx);
        return NULL;
    }

    VisitCtx ctx = { ir, header_path };
    CXCursor root = g_clang.getTranslationUnitCursor(tu);
    g_clang.visitChildren(root, visit_fn, &ctx);

    g_clang.disposeTranslationUnit(tu);
    g_clang.disposeIndex(idx);
    return ir;
}
