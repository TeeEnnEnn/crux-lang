#ifndef COMPILER_EXPRESSIONS_H
#define COMPILER_EXPRESSIONS_H


#include "compiler_core.h"

void or_(Compiler *compiler, const bool can_assign);
void and_(Compiler *compiler, const bool can_assign);
void array_literal(Compiler* compiler, const bool can_assign);
void set_literal(Compiler *compiler, const bool can_assign);
void tuple_literal(Compiler *compiler, const bool can_assign);
void table_literal(Compiler *compiler, const bool can_assign);
void collection_index(Compiler *compiler, const bool can_assign);
void result_unwrap(Compiler *compiler, const bool can_assign);
void number(Compiler *compiler, bool can_assign);
void string(Compiler *compiler, const bool can_assign);
void unary(Compiler *compiler, const bool can_assign);
void typeof_expression(Compiler *compiler, const bool can_assign);
void ok_expression(Compiler *compiler, const bool can_assign);
void err_expression(Compiler *compiler, const bool can_assign);
void some_expression(Compiler *compiler, const bool can_assign);
void none_expression(Compiler *compiler, const bool can_assign);
void type_coerce(Compiler *compiler, const bool can_assign);
void binary(Compiler *compiler, bool can_assign);

void infix_call(Compiler *compiler, const bool can_assign);
void dot(Compiler *compiler, const bool can_assign);

void struct_instance(Compiler *compiler, const bool can_assign);

/**
 * Parses a named variable (local, upvalue, or global).
 * pushes the type of the variable onto the type stack.
 *
 * @param compiler The current compiler
 * @param name The token representing the variable name.
 * @param can_assign Whether the variable expression can be the target of an
 * assignment.
 */
void named_variable(Compiler *compiler, Token name, const bool can_assign);

/**
 * Parses an expression.
 * Allocates - so protect before
 */
void expression(Compiler *compiler);

/**
 * Starts at the current token and parses any expression at the given precedence or
 * * higher
 * @param compiler The current compiler
 * @param precedence The precedence to parse at or higher
 */
void parse_precedence(Compiler *compiler, const Precedence precedence);

ParseRule *get_rule(const CruxTokenType type);

#endif // COMPILER_EXPRESSIONS_H
