#ifndef COMPILER_DECLARATIONS_H
#define COMPILER_DECLARATIONS_H

#include "vm.h"

void public_declaration(Compiler *compiler);
void type_declaration(Compiler *compiler, bool is_public);
void impl_declaration(Compiler *compiler);
void var_declaration(Compiler *compiler, const bool is_public);

void declaration(Compiler *compiler);

#endif // COMPILER_DECLARATIONS_H
