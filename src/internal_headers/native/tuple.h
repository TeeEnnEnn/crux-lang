#include "object.h"
#include "value.h"

CruxValue new_tuple_function(CruxVM *vm, const CruxValue *args);

CruxValue is_empty_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue contains_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue to_array_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue first_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue last_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue equals_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue get_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue slice_tuple_method(CruxVM *vm, const CruxValue *args);
CruxValue index_tuple_method(CruxVM *vm, const CruxValue *args);
