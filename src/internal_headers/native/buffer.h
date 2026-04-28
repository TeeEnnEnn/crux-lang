#include "object.h"
#include "value.h"

CruxValue new_buffer_function(CruxVM *vm, const CruxValue *args);

CruxValue write_byte_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_int16_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_int32_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_float32_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_float64_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_int16_be_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_int32_be_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_float32_be_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_float64_be_buffer_method(CruxVM *vm, const CruxValue *args);

CruxValue write_string_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue write_buffer_buffer_method(CruxVM *vm, const CruxValue *args);

CruxValue read_byte_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_string_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_line_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_all_buffer_method(CruxVM *vm, const CruxValue *args);

CruxValue read_int16_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_int32_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_float32_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_float64_le_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_int16_be_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_int32_be_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_float32_be_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue read_float64_be_buffer_method(CruxVM *vm, const CruxValue *args);

CruxValue capacity_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue is_empty_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue clear_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue peek_byte_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue skip_bytes_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue clone_buffer_method(CruxVM *vm, const CruxValue *args);
CruxValue compact_buffer_method(CruxVM *vm, const CruxValue *args);
