#ifndef CRUX_GC_H
#define CRUX_GC_H

#include "object.h"

CruxValue gc_off_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_on_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_set_heap_growth_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_set_min_heap_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_set_min_growth_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_collect_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_heap_used_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_heap_capacity_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_is_on_function(CruxVM *vm, const CruxValue *args);
CruxValue gc_stats_function(CruxVM *vm, const CruxValue *args);

#endif
