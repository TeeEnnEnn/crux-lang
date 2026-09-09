#ifndef COMPILER_FUNCTIONS_H
#define COMPILER_FUNCTIONS_H

#include "compiler/compiler_core.h"
#include "vm.h"

void function(Compiler *compiler, const FunctionType type, ObjectTypeRecord *self_type, ObjectString *recursive_name,
			  int recursive_global_index);

void fn_declaration(Compiler *compiler, const bool is_public);
void native_declaration(Compiler *compiler, const bool is_public);

void anonymous_function(Compiler *compiler, const bool can_assign);

#endif // COMPILER_FUNCTIONS_H
