#ifndef PARSER_H
#define PARSER_H

#include "ir.h"

CinPyIR *cinpy_parse_header(const char *header_path,
                             const char **extra_args, int n_args,
                             const char *clang_include_path);

#endif
