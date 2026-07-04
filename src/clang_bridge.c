#include "clang_bridge.h"
#include <stdio.h>
#include <string.h>

ClangBridge g_clang = {0};

#ifdef _WIN32
#define CINPY_DLOPEN(path)      LoadLibraryA(path)
#define CINPY_DLSYM(h, name)    ((void *)GetProcAddress((HMODULE)(h), (name)))
#define CINPY_DLCLOSE(h)        FreeLibrary((HMODULE)(h))
#else
#define CINPY_DLOPEN(path)      dlopen((path), RTLD_LAZY | RTLD_GLOBAL)
#define CINPY_DLSYM(h, name)    dlsym((h), (name))
#define CINPY_DLCLOSE(h)        dlclose(h)
#endif

#define LOAD_SYM(bridge, name, field) \
    do { \
        (bridge)->field = (pfn_##field) CINPY_DLSYM((bridge)->handle, name); \
        if (!(bridge)->field) { \
            fprintf(stderr, "CinPy: failed to resolve %s from libclang\n", name); \
            CINPY_DLCLOSE((bridge)->handle); \
            (bridge)->handle = NULL; \
            return -1; \
        } \
    } while (0)

int clang_bridge_init(ClangBridge *b, const char *dll_path)
{
    if (b->handle)
        return 0;

    b->handle = CINPY_DLOPEN(dll_path);
    if (!b->handle) {
        fprintf(stderr, "CinPy: could not load libclang from '%s'\n", dll_path);
        return -1;
    }

    LOAD_SYM(b, "clang_createIndex",                createIndex);
    LOAD_SYM(b, "clang_disposeIndex",               disposeIndex);
    LOAD_SYM(b, "clang_parseTranslationUnit",        parseTranslationUnit);
    LOAD_SYM(b, "clang_disposeTranslationUnit",      disposeTranslationUnit);
    LOAD_SYM(b, "clang_getTranslationUnitCursor",    getTranslationUnitCursor);
    LOAD_SYM(b, "clang_visitChildren",               visitChildren);
    LOAD_SYM(b, "clang_getCursorKind",               getCursorKind);
    LOAD_SYM(b, "clang_getCursorSpelling",           getCursorSpelling);
    LOAD_SYM(b, "clang_getCursorType",               getCursorType);
    LOAD_SYM(b, "clang_getResultType",               getResultType);
    LOAD_SYM(b, "clang_getNumArgTypes",              getNumArgTypes);
    LOAD_SYM(b, "clang_getArgType",                  getArgType);
    LOAD_SYM(b, "clang_getCString",                  getCString);
    LOAD_SYM(b, "clang_disposeString",               disposeString);
    LOAD_SYM(b, "clang_getTypeSpelling",             getTypeSpelling);
    LOAD_SYM(b, "clang_getCursorLocation",           getCursorLocation);
    LOAD_SYM(b, "clang_getExpansionLocation",        getExpansionLocation);
    LOAD_SYM(b, "clang_getFileName",                 getFileName);

    return 0;
}

void clang_bridge_close(ClangBridge *b)
{
    if (b->handle) {
        CINPY_DLCLOSE(b->handle);
        memset(b, 0, sizeof(*b));
    }
}
