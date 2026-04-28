#ifndef MATH_H
#define MATH_H

#include "value.h"

CruxValue pow_function(CruxVM *vm, const CruxValue *args);
CruxValue sqrt_function(CruxVM *vm, const CruxValue *args);
CruxValue abs_function(CruxVM *vm, const CruxValue *args);

CruxValue sin_function(CruxVM *vm, const CruxValue *args);
CruxValue cos_function(CruxVM *vm, const CruxValue *args);
CruxValue tan_function(CruxVM *vm, const CruxValue *args);

CruxValue asin_function(CruxVM *vm, const CruxValue *args);
CruxValue acos_function(CruxVM *vm, const CruxValue *args);
CruxValue atan_function(CruxVM *vm, const CruxValue *args);

CruxValue exp_function(CruxVM *vm, const CruxValue *args);
CruxValue ln_function(CruxVM *vm, const CruxValue *args);
CruxValue log10_function(CruxVM *vm, const CruxValue *args);

CruxValue ceil_function(CruxVM *vm, const CruxValue *args);
CruxValue floor_function(CruxVM *vm, const CruxValue *args);
CruxValue round_function(CruxVM *vm, const CruxValue *args);

CruxValue min_function(CruxVM *vm, const CruxValue *args);
CruxValue max_function(CruxVM *vm, const CruxValue *args);

CruxValue pi_function(CruxVM *vm, const CruxValue *args);
CruxValue e_function(CruxVM *vm, const CruxValue *args);
CruxValue nan_function(CruxVM *vm, const CruxValue *args);
CruxValue inf_function(CruxVM *vm, const CruxValue *args);

#endif // MATH_H
