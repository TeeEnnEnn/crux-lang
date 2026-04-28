#include "object.h"
#include "value.h"

CruxValue new_set_function(CruxVM *vm, const CruxValue *args);
CruxValue add_set_method(CruxVM *vm, const CruxValue *args);
CruxValue remove_set_method(CruxVM *vm, const CruxValue *args);
CruxValue discard_set_method(CruxVM *vm, const CruxValue *args);
CruxValue union_set_method(CruxVM *vm, const CruxValue *args);
CruxValue intersection_set_method(CruxVM *vm, const CruxValue *args);
CruxValue difference_set_method(CruxVM *vm, const CruxValue *args);
CruxValue sym_difference_set_method(CruxVM *vm, const CruxValue *args);
CruxValue is_subset_set_method(CruxVM *vm, const CruxValue *args);
CruxValue is_superset_set_method(CruxVM *vm, const CruxValue *args);
CruxValue is_disjoint_set_method(CruxVM *vm, const CruxValue *args);

CruxValue contains_set_method(CruxVM *vm, const CruxValue *args);
CruxValue is_empty_set_method(CruxVM *vm, const CruxValue *args);
CruxValue to_array_set_method(CruxVM *vm, const CruxValue *args);
CruxValue clone_set_method(CruxVM *vm, const CruxValue *args);
