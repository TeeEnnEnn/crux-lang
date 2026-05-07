#ifndef ERROR_H
#define ERROR_H

#include "object/object.h"
#include "value.h"

CruxValue error_function(CruxVM *vm, const CruxValue *args);
CruxValue panic_function(CruxVM *vm, const CruxValue *args);
CruxValue assert_function(CruxVM *vm, const CruxValue *args);
CruxValue error_type_method(CruxVM *vm, const CruxValue *args);
CruxValue error_message_method(CruxVM *vm, const CruxValue *args);

#endif // ERROR_H
