#ifndef EMITTER_H
#define EMITTER_H

#include "ir.h"
#include "cpyc.h"

CpycStatus cinpy_emit_cpyc(const CinPyIR *ir, const char *out_path);

#endif
