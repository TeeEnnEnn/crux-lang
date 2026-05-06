#ifndef RANDOM_H
#define RANDOM_H

#include "object/object.h"
#include "vm.h"

CruxValue random_seed_method(CruxVM *vm, const CruxValue *args);
CruxValue random_int_method(CruxVM *vm, const CruxValue *args);
CruxValue random_float_method(CruxVM *vm, const CruxValue *args);
CruxValue random_bool_method(CruxVM *vm, const CruxValue *args);
CruxValue random_choice_method(CruxVM *vm, const CruxValue *args);

CruxValue random_next_method(CruxVM *vm, const CruxValue *args);
CruxValue random_init_function(CruxVM *vm, const CruxValue *args);
#endif // RANDOM_H
