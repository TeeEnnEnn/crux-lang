#ifndef CRUX_COMPILER_STATEMENTS_H
#define CRUX_COMPILER_STATEMENTS_H

#include "vm.h"

void if_statement(Compiler *compiler);
void block(Compiler *compiler);
void while_statement(Compiler *compiler);
void for_statement(Compiler *compiler);
void return_statement(Compiler *compiler);
void use_statement(Compiler *compiler, bool is_public);
void continue_statement(Compiler *compiler);
void break_statement(Compiler *compiler);
void panic_statement(Compiler *compiler);
void expression_statement(Compiler *compiler);
void give_statement(Compiler *compiler);

void statement(Compiler *compiler);

#endif // CRUX_COMPILER_STATEMENTS_H
