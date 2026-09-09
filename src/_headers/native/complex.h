#ifndef CRUX_LANG_COMPLEX_H
#define CRUX_LANG_COMPLEX_H

#include "object/object.h"
#include "value.h"
#include "vm.h"

CruxValue complex_real_method(CruxVM *vm, const CruxValue *args);
CruxValue complex_imag_method(CruxVM *vm, const CruxValue *args);
CruxValue conjugate_complex_number_method(CruxVM *vm, const CruxValue *args);
CruxValue magnitude_complex_number_method(CruxVM *vm, const CruxValue *args);
CruxValue square_magnitude_complex_number_method(CruxVM *vm, const CruxValue *args);

CruxValue new_complex_function(CruxVM *vm, const CruxValue *args);

CruxValue complex_add_value(CruxVM *vm, const ObjectComplex *lhs, const ObjectComplex *rhs);
CruxValue complex_subtract_value(CruxVM *vm, const ObjectComplex *lhs, const ObjectComplex *rhs);
CruxValue complex_multiply_value(CruxVM *vm, const ObjectComplex *lhs, const ObjectComplex *rhs);
CruxValue complex_divide_value(CruxVM *vm, const ObjectComplex *lhs, const ObjectComplex *rhs);
CruxValue complex_scalar_multiply_value(CruxVM *vm, const ObjectComplex *value, double scalar);
CruxValue complex_scalar_divide_value(CruxVM *vm, const ObjectComplex *value, double scalar);

CruxValue add_complex_number_method(CruxVM *vm, const CruxValue *args);
CruxValue sub_complex_number_method(CruxVM *vm, const CruxValue *args);
CruxValue mul_complex_number_method(CruxVM *vm, const CruxValue *args);
CruxValue div_complex_number_method(CruxVM *vm, const CruxValue *args);
CruxValue scale_complex_number_method(CruxVM *vm, const CruxValue *args);

#endif // CRUX_LANG_COMPLEX_H
