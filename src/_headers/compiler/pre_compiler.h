#ifndef PRE_COMPILER_H
#define PRE_COMPILER_H

#include "vm.h"

// Run both pre-scan sub-passes and merge results into `dest`.
// The scanner must be initialized before calling.
void pre_scan(Compiler *compiler, char *source, ObjectTypeTable *dest);

#endif // PRE_COMPILER_H
