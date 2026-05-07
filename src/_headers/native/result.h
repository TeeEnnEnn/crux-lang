#ifndef CRUX_STDLIB_RESULT_H
#define CRUX_STDLIB_RESULT_H

#include "value.h"

CruxValue result_unwrap_method(CruxVM *vm, const CruxValue *args);
CruxValue result_is_ok_method(CruxVM *vm, const CruxValue *args);
CruxValue result_is_err_method(CruxVM *vm, const CruxValue *args);
CruxValue result_unwrap_or_method(CruxVM *vm, const CruxValue *args);

#endif // CRUX_STDLIB_RESULT_H
