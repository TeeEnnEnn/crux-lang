#ifndef CRUX_STDLIB_OPTION_H
#define CRUX_STDLIB_OPTION_H

#include "value.h"

CruxValue option_is_some_method(CruxVM *vm, const CruxValue *args);
CruxValue option_is_none_method(CruxVM *vm, const CruxValue *args);
CruxValue option_unwrap_method(CruxVM *vm, const CruxValue *args);
CruxValue option_unwrap_or_method(CruxVM *vm, const CruxValue *args);

#endif // CRUX_STDLIB_OPTION_H
