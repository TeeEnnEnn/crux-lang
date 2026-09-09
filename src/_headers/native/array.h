#ifndef ARRAY_H
#define ARRAY_H

#include "value.h"
#include "vm.h"

CruxValue array_push_method(CruxVM *vm,
							const CruxValue *args); // [1,2].push( 3) -> [1,2,3]
CruxValue array_pop_method(CruxVM *vm,
						   const CruxValue *args); // [1,2,3].pop([1,2,3]) -> 3, [1,2]
CruxValue array_insert_method(CruxVM *vm,
							  const CruxValue *args); // [1,3].insert(1, 2) -> [1,2,3]
CruxValue array_remove_at_method(CruxVM *vm,
								 const CruxValue *args); // [1,2,3].remove_at(1) -> [1,3]
CruxValue array_concat_method(CruxVM *vm,
							  const CruxValue *args); // [1,2].concat([3,4]) -> [1,2,3,4]
CruxValue array_slice_method(CruxVM *vm,
							 const CruxValue *args); // [1,2,3].slice(1, 2) -> [2]
CruxValue array_reverse_method(CruxVM *vm,
							   const CruxValue *args); // [1,2,3].reverse([1,2,3]) -> [3,2,1]
CruxValue array_index_of_method(CruxVM *vm,
								const CruxValue *args); // [1,2,3].index_of(2) -> 1
CruxValue array_contains_method(CruxVM *vm,
								const CruxValue *args); // [1,2,3].contains(2) -> true
CruxValue array_clear_method(CruxVM *vm,
							 const CruxValue *args); // [1,2,3].clear([1,2,3]) -> []
CruxValue arrayEqualsMethod(CruxVM *vm,
							const CruxValue *args); // [1,2,3].equals([1,2,3]) -> true

CruxValue array_map_method(CruxVM *vm,
						   const CruxValue *args); // [1,2,3].map(fn (x){ return x * 2;}) -> [2,4,6]

CruxValue array_filter_method(CruxVM *vm,
							  const CruxValue *args); // [1,2,3].filter(fn (x){ return x % 2 == 0;}) -> [2]

CruxValue array_reduce_method(CruxVM *vm,
							  const CruxValue *args); // [1,2,3].reduce(fn (acc, x){ return acc + x;}, 0) -> 6

CruxValue array_sort_method(CruxVM *vm,
							const CruxValue *args); // [1,2,3].sort() -> [1,2,3]

CruxValue array_join_method(CruxVM *vm,
							const CruxValue *args); // [1, 2, 3].join("") -> "123"

#endif // ARRAY_H
