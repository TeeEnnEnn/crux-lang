#ifndef VECTORS_H
#define VECTORS_H

#include "object/object.h"
#include "value.h"

CruxValue new_vector_function(CruxVM *vm, const CruxValue *args);

CruxValue vector_dot_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_add_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_subtract_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_multiply_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_divide_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_magnitude_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_normalize_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_distance_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_cross_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_angle_between_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_lerp_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_reflect_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_equals_method(CruxVM *vm, const CruxValue *args);

CruxValue vector_x_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_y_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_z_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_w_method(CruxVM *vm, const CruxValue *args);
CruxValue vector_dimension_method(CruxVM *vm, const CruxValue *args);

#endif // VECTORS_H
