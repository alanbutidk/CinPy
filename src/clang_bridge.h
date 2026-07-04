#ifndef CLANG_BRIDGE_H
#define CLANG_BRIDGE_H

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE cinpy_dl_handle;
#else
#include <dlfcn.h>
typedef void *cinpy_dl_handle;
#endif

#include "clang-c/Index.h"

typedef CXIndex (*pfn_createIndex)(int, int);
typedef void (*pfn_disposeIndex)(CXIndex);
typedef CXTranslationUnit (*pfn_parseTranslationUnit)(CXIndex, const char *,
                                                      const char *const *, int,
                                                      struct CXUnsavedFile *,
                                                      unsigned, unsigned);
typedef void (*pfn_disposeTranslationUnit)(CXTranslationUnit);
typedef CXCursor (*pfn_getTranslationUnitCursor)(CXTranslationUnit);
typedef enum CXChildVisitResult (*pfn_visitChildren_cb)(CXCursor, CXCursor,
                                                        CXClientData);
typedef unsigned (*pfn_visitChildren)(CXCursor, CXCursorVisitor, CXClientData);
typedef enum CXCursorKind (*pfn_getCursorKind)(CXCursor);
typedef CXString (*pfn_getCursorSpelling)(CXCursor);
typedef CXType (*pfn_getCursorType)(CXCursor);
typedef CXType (*pfn_getResultType)(CXType);
typedef int (*pfn_getNumArgTypes)(CXType);
typedef CXType (*pfn_getArgType)(CXType, unsigned);
typedef enum CXTypeKind (*pfn_getTypeKind)(CXType);
typedef const char *(*pfn_getCString)(CXString);
typedef void (*pfn_disposeString)(CXString);
typedef CXString (*pfn_getTypeSpelling)(CXType);
typedef CXSourceLocation (*pfn_getCursorLocation)(CXCursor);
typedef void (*pfn_getExpansionLocation)(CXSourceLocation, CXFile *, unsigned *,
                                         unsigned *, unsigned *);
typedef CXString (*pfn_getFileName)(CXFile);

typedef struct {
  cinpy_dl_handle handle;
  pfn_createIndex createIndex;
  pfn_disposeIndex disposeIndex;
  pfn_parseTranslationUnit parseTranslationUnit;
  pfn_disposeTranslationUnit disposeTranslationUnit;
  pfn_getTranslationUnitCursor getTranslationUnitCursor;
  pfn_visitChildren visitChildren;
  pfn_getCursorKind getCursorKind;
  pfn_getCursorSpelling getCursorSpelling;
  pfn_getCursorType getCursorType;
  pfn_getResultType getResultType;
  pfn_getNumArgTypes getNumArgTypes;
  pfn_getArgType getArgType;
  pfn_getCString getCString;
  pfn_disposeString disposeString;
  pfn_getTypeSpelling getTypeSpelling;
  pfn_getCursorLocation getCursorLocation;
  pfn_getExpansionLocation getExpansionLocation;
  pfn_getFileName getFileName;
} ClangBridge;

int clang_bridge_init(ClangBridge *b, const char *dll_path);
void clang_bridge_close(ClangBridge *b);

extern ClangBridge g_clang;

#endif
