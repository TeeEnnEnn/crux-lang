#ifndef STRING_H
#define STRING_H

#include "object/object.h"

CruxValue string_byte_length_method(CruxVM *vm, const CruxValue *args);
CruxValue string_first_method(CruxVM *vm, const CruxValue *args);
CruxValue string_last_method(CruxVM *vm, const CruxValue *args);
CruxValue string_get_method(CruxVM *vm, const CruxValue *args);
CruxValue string_is_upper_method(CruxVM *vm, const CruxValue *args);
CruxValue string_is_lower_method(CruxVM *vm, const CruxValue *args);
CruxValue string_strip_method(CruxVM *vm, const CruxValue *args);
CruxValue string_substring_method(CruxVM *vm, const CruxValue *args);
CruxValue string_replace_method(CruxVM *vm, const CruxValue *args);
CruxValue string_split_method(CruxVM *vm, const CruxValue *args);
CruxValue string_contains_method(CruxVM *vm, const CruxValue *args);
CruxValue string_starts_with_method(CruxVM *vm, const CruxValue *args);
CruxValue string_ends_with_method(CruxVM *vm, const CruxValue *args);
CruxValue string_concat_method(CruxVM *vm, const CruxValue *args);

CruxValue string_to_upper_method(CruxVM *vm, const CruxValue *args);
CruxValue string_to_lower_method(CruxVM *vm, const CruxValue *args);

CruxValue string_is_al_num_method(CruxVM *vm, const CruxValue *args);
CruxValue string_is_alpha_method(CruxVM *vm, const CruxValue *args);
CruxValue string_is_digit_method(CruxVM *vm, const CruxValue *args);
CruxValue string_reverse_method(CruxVM *vm, const CruxValue *args);
CruxValue string_find_method(CruxVM *vm, const CruxValue *args);
CruxValue string_repeat_method(CruxVM *vm, const CruxValue *args);
CruxValue string_join_method(CruxVM *vm, const CruxValue *args);

CruxValue string_pad_left_method(CruxVM *vm, const CruxValue *args);
CruxValue string_pad_right_method(CruxVM *vm, const CruxValue *args);
CruxValue string_count_method(CruxVM *vm, const CruxValue *args);
CruxValue string_is_empty_method(CruxVM *vm, const CruxValue *args);
CruxValue string_is_space_method(CruxVM *vm, const CruxValue *args);
#endif // STRING_H
