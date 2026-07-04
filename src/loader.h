#ifndef LOADER_H
#define LOADER_H

#include "ir.h"
#include "cpyc.h"

CpycStatus cinpy_load_cpyc(const char *path, CinPyIR **out);

#endif
