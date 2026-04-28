#include "object.h"
#include "value.h"

bool validate_range_values(int32_t start, int32_t step, int32_t end, const char **error_message);

CruxValue new_range_function(CruxVM *vm, const CruxValue *args);

CruxValue contains_range_method(CruxVM *vm, const CruxValue *args);
CruxValue to_array_range_method(CruxVM *vm, const CruxValue *args);
CruxValue start_range_method(CruxVM *vm, const CruxValue *args);
CruxValue end_range_method(CruxVM *vm, const CruxValue *args);
CruxValue step_range_method(CruxVM *vm, const CruxValue *args);
CruxValue is_empty_range_method(CruxVM *vm, const CruxValue *args);
CruxValue reversed_range_method(CruxVM *vm, const CruxValue *args);
