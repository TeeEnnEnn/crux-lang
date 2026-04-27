#ifndef COMPILER_HELPERS_H
#define COMPILER_HELPERS_H

#include "object.h"
#include "vm.h"
#include "scanner.h"

/**
 * lookup a callable in a vm stdlib table
 * @param compiler The current compiler
 * @param type_table The vm owned type table to look for a callable from
 * @param name_token The name of the callable
 * @return the callable otherwise NULL if no callable found
 */
const ObjectNativeCallable *lookup_stdlib_method(const Compiler *compiler, const Table *type_table,
												 const Token *name_token);
void consume_identifier_like(const Compiler *compiler, const char *message);
bool check_identifier_like(const Compiler *compiler);
bool is_identifier_like(CruxTokenType type);
bool match(const Compiler *compiler, CruxTokenType type);
bool check(const Compiler *compiler, CruxTokenType type);
void advance(const Compiler *compiler);
void consume(const Compiler *compiler, CruxTokenType type, const char *message);
Chunk *current_chunk(const Compiler *compiler);

int merge_vector_dimensions(Compiler *compiler, const ObjectTypeRecord *left_type,
								   const ObjectTypeRecord *right_type, const char *operation);

bool is_primitive_numeric_type(const ObjectTypeRecord *type);

ObjectTypeRecord *merge_matrix_shape(Compiler *compiler, const ObjectTypeRecord *left_type,
											const ObjectTypeRecord *right_type, const char *operation);

ObjectTypeRecord *matrix_multiply_result_type(Compiler *compiler, const ObjectTypeRecord *left_type,
													 const ObjectTypeRecord *right_type);

bool resolve_assignment_target(Compiler *compiler, const Token name, uint16_t *set_op, int *arg,
									  ObjectTypeRecord **target_type);

Token peek_next_token(const Compiler *compiler);

/**
 * Synchronizes the parser after encountering a syntax error.
 *
 * Discards tokens until a statement boundary is found to minimize cascading
 * errors.
 */
void synchronize(const Compiler *compiler);

ObjectModuleRecord *compile_module_statically(Compiler *compiler, ObjectString *path);

bool parse_signed_int_literal(Compiler *compiler, int32_t *value, const char *message);

bool match_type_name(const Compiler *compiler);

#endif // COMPILER_HELPERS_H
