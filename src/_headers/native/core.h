#ifndef CORE_H
#define CORE_H

#include "value.h"

CruxValue length_function(CruxVM *vm, const CruxValue *args);
CruxValue int_function(CruxVM *vm, const CruxValue *args);
CruxValue float_function(CruxVM *vm, const CruxValue *args);
CruxValue string_function(CruxVM *vm, const CruxValue *args);
CruxValue array_function(CruxVM *vm, const CruxValue *args);
CruxValue table_function(CruxVM *vm, const CruxValue *args);
CruxValue format_function(CruxVM *vm, const CruxValue *args);

CruxValue iter_function(CruxVM *vm, const CruxValue *args);
CruxValue next_function(CruxVM *vm, const CruxValue *args);

#endif // CORE_H
