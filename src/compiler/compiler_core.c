#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler/compiler_core.h"
#include "debug.h"
#include "garbage_collector.h"
#include "object.h"
#include "panic.h"
#include "scanner.h"
#include "table.h"
#include "type_system.h"
#include "value.h"
#include "vm.h"

#include "compiler/compiler_declarations.h"
#include "compiler/compiler_helpers.h"
#include "compiler/pre_compiler.h"

ObjectTypeRecord *parse_type_record(Compiler *compiler)
{
	ObjectTypeRecord *type_record = NULL;

	if (match(compiler, CRUX_TOKEN_INT_TYPE)) {
		type_record = T_INT;
	} else if (match(compiler, CRUX_TOKEN_FLOAT_TYPE)) {
		type_record = T_FLOAT;
	} else if (match(compiler, CRUX_TOKEN_BOOL_TYPE)) {
		type_record = T_BOOL;
	} else if (match(compiler, CRUX_TOKEN_STRING_TYPE)) {
		type_record = T_STRING;
	} else if (match(compiler, CRUX_TOKEN_NIL_TYPE)) {
		type_record = T_NIL;
	} else if (match(compiler, CRUX_TOKEN_ANY_TYPE)) {
		type_record = T_ANY;
	} else if (match(compiler, CRUX_TOKEN_SHAPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_BRACE)) {
			int field_count = 0;
			ObjectTypeTable *field_types = new_type_table(compiler->owner, 8);
			push(compiler->owner->current_module_record, OBJECT_VAL(field_types));
			if (!check(compiler, CRUX_TOKEN_RIGHT_BRACE)) {
				do {
					consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected field name.");
					ObjectString *fieldName = copy_string(compiler->owner, compiler->parser->previous.start,
														  compiler->parser->previous.length);
					push(compiler->owner->current_module_record, OBJECT_VAL(fieldName));
					consume(compiler, CRUX_TOKEN_COLON, "Expected ':' after field name.");
					ObjectTypeRecord *field_type = parse_type_record(compiler);
					push(compiler->owner->current_module_record, OBJECT_VAL(field_type));
					type_table_set(field_types, fieldName, field_type);
					field_count++;
				} while (match(compiler, CRUX_TOKEN_COMMA));
			}
			consume(compiler, CRUX_TOKEN_RIGHT_BRACE, "Expected '}' after shape definition.");
			type_record = new_shape_type_rec(compiler->owner, field_types, field_count);

			for (int i = 0; i < field_count; i++) {
				pop(compiler->owner->current_module_record); // field type
				pop(compiler->owner->current_module_record); // field name
			}
			pop(compiler->owner->current_module_record); // field_types

		} else {
			compiler_panic(compiler->parser, "Expected '{' for shape type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_ARRAY_TYPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			type_record = new_type_rec(compiler->owner, ARRAY_TYPE);
			push(compiler->owner->current_module_record, OBJECT_VAL(type_record));
			ObjectTypeRecord *parsed_type = parse_type_record(compiler);
			type_record->as.array_type.element_type = parsed_type;
			consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after array element type.");
			pop(compiler->owner->current_module_record); // type_record
		} else {
			compiler_panic(compiler->parser, "Expected '[' for array type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_TABLE_TYPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			type_record = new_type_rec(compiler->owner, TABLE_TYPE);
			push(compiler->owner->current_module_record, OBJECT_VAL(type_record));
			ObjectTypeRecord *key_type = parse_type_record(compiler);
			push(compiler->owner->current_module_record, OBJECT_VAL(key_type));

			if (!is_valid_table_key_type(key_type)) {
				pop(compiler->owner->current_module_record); // key_type
				pop(compiler->owner->current_module_record); // type_record
				char got[128];
				type_record_name(key_type, got, sizeof(got));
				compiler_panicf(
					compiler->parser, TYPE,
					"Table key type '%s' is not hashable. Keys must be 'Int | Float | String | Bool | Nil'.", got);
				return T_ANY;
			}

			type_record->as.table_type.key_type = key_type;
			consume(compiler, CRUX_TOKEN_COMMA, "Expected ',' after key type.");
			type_record->as.table_type.value_type = parse_type_record(compiler);
			consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after table element type.");
			pop(compiler->owner->current_module_record); // key_type
			pop(compiler->owner->current_module_record); // type_record
		} else {
			compiler_panic(compiler->parser, "Expected '[' for table type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_ITERATOR_TYPE)) {
		if (!match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			compiler_panic(compiler->parser, "Expected '[' for iterator type definition.", TYPE);
			type_record = T_ANY;
		}
		ObjectTypeRecord *element_type = parse_type_record(compiler);
		if (!match(compiler, CRUX_TOKEN_RIGHT_SQUARE)) {
			compiler_panic(compiler->parser, "Expected ']' after iterator element type.", TYPE);
		}
		push(compiler->owner->current_module_record, OBJECT_VAL(element_type));
		type_record = new_iterator_type_rec(compiler->owner, element_type);
		pop(compiler->owner->current_module_record); // element_type
	} else if (match(compiler, CRUX_TOKEN_VECTOR_TYPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			type_record = new_type_rec(compiler->owner, VECTOR_TYPE);
			push(compiler->owner->current_module_record, OBJECT_VAL(type_record));

			if (match(compiler, CRUX_TOKEN_RIGHT_SQUARE)) {
				pop(compiler->owner->current_module_record); // type_record
				type_record->as.vector_type.dimensions = -1;
				return type_record;
			}

			consume(compiler, CRUX_TOKEN_INT, "Expected 'int' for vector dimensions.");
			const int dimensions = (int)strtol(compiler->parser->previous.start, NULL, 10);
			type_record->as.vector_type.dimensions = dimensions;
			consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after vector element type.");
			pop(compiler->owner->current_module_record); // type_record
		} else {
			compiler_panic(compiler->parser, "Expected '[' for vector type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_MATRIX_TYPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			type_record = new_type_rec(compiler->owner, MATRIX_TYPE);
			push(compiler->owner->current_module_record, OBJECT_VAL(type_record));

			if (match(compiler, CRUX_TOKEN_COMMA)) {
				if (match(compiler, CRUX_TOKEN_RIGHT_SQUARE)) {
					type_record->as.matrix_type.cols = -1;
					type_record->as.matrix_type.rows = -1;
					pop(compiler->owner->current_module_record); // type_record
					return type_record;
				} else {
					pop(compiler->owner->current_module_record); // type_record
					compiler_panic(compiler->parser, "Expected ']' to end definition of generic matrix type", SYNTAX);
					return T_ANY;
				}
			}

			consume(compiler, CRUX_TOKEN_INT, "Expected 'int' for matrix dimensions.");
			const int row_dim = (int)strtol(compiler->parser->previous.start, NULL, 10);
			consume(compiler, CRUX_TOKEN_COMMA, "Expected ',' after row dimension.");
			const int col_dim = (int)strtol(compiler->parser->previous.start, NULL, 10);
			type_record->as.matrix_type.rows = row_dim;
			type_record->as.matrix_type.cols = col_dim;
			consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after matrix element type.");
			pop(compiler->owner->current_module_record); // type_record
		} else {
			compiler_panic(compiler->parser, "Expected '[' for matrix type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_BUFFER_TYPE)) {
		type_record = new_type_rec(compiler->owner, BUFFER_TYPE);
	} else if (match(compiler, CRUX_TOKEN_ERROR_TYPE)) {
		type_record = new_type_rec(compiler->owner, ERROR_TYPE);
	} else if (match(compiler, CRUX_TOKEN_RESULT_TYPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			ObjectTypeRecord *value_type = parse_type_record(compiler);
			push(compiler->owner->current_module_record, OBJECT_VAL(value_type));
			consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after result value type.");
			type_record = new_result_type_rec(compiler->owner, value_type);
			pop(compiler->owner->current_module_record); // value_type
		} else {
			compiler_panic(compiler->parser, "Expected '[' for result type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_OPTION_TYPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			ObjectTypeRecord *some_type = parse_type_record(compiler);
			push(compiler->owner->current_module_record, OBJECT_VAL(some_type));
			consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after option some type.");
			type_record = new_option_type_rec(compiler->owner, some_type);
			pop(compiler->owner->current_module_record); // some_type
		} else {
			compiler_panic(compiler->parser, "Expected '[' for option type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_RANGE_TYPE)) {
		type_record = new_type_rec(compiler->owner, RANGE_TYPE);
	} else if (match(compiler, CRUX_TOKEN_TUPLE_TYPE)) {
		if (match(compiler, CRUX_TOKEN_LEFT_SQUARE)) {
			int param_capacity = 4;
			int param_count = 0;
			ObjectTypeRecord **param_types = ALLOCATE(compiler->owner, ObjectTypeRecord *, param_capacity);
			if (!param_types) {
				compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
				return T_ANY;
			}

			if (!check(compiler, CRUX_TOKEN_RIGHT_SQUARE)) {
				do {
					if (param_count == param_capacity) {
						int old_capacity = param_capacity;
						param_capacity = GROW_CAPACITY(param_capacity);
						ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types,
															  old_capacity, param_capacity);
						if (!grown) {
							FREE_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, old_capacity);
							for (int i = 0; i < param_count; i++)
								pop(compiler->owner->current_module_record);
							compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
							return T_ANY;
						}
						param_types = grown;
					}
					ObjectTypeRecord *param_type = parse_type_record(compiler);
					push(compiler->owner->current_module_record, OBJECT_VAL(param_type));
					param_types[param_count++] = param_type;
				} while (match(compiler, CRUX_TOKEN_COMMA));
			}
			param_types = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_capacity, param_count);
			type_record = new_tuple_type_rec(compiler->owner, param_types, param_count);
			for (int i = 0; i < param_count; i++) {
				pop(compiler->owner->current_module_record); // param_types[i]
			}
			consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after tuple element types.");
		} else {
			compiler_panic(compiler->parser, "Expected '[' for tuple type definition.", TYPE);
			type_record = T_ANY;
		}
	} else if (match(compiler, CRUX_TOKEN_COMPLEX_TYPE)) {
		type_record = new_type_rec(compiler->owner, COMPLEX_TYPE);
	} else if (match(compiler, CRUX_TOKEN_LEFT_PAREN)) {
		int param_capacity = 4;
		int param_count = 0;
		ObjectTypeRecord **param_types = ALLOCATE(compiler->owner, ObjectTypeRecord *, param_capacity);
		if (!param_types) {
			compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
			return T_ANY;
		}

		if (!check(compiler, CRUX_TOKEN_RIGHT_PAREN)) {
			do {
				ObjectTypeRecord *inner = parse_type_record(compiler);
				if (!inner) {
					compiler_panic(compiler->parser, "Expected type.", TYPE);
					inner = T_ANY;
				}

				// protect inner before array grows
				push(compiler->owner->current_module_record, OBJECT_VAL(inner));

				if (param_count == param_capacity) {
					param_capacity = GROW_CAPACITY(param_capacity);
					ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_count,
														  param_capacity);
					if (!grown) {
						FREE_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_count);
						// +1: account for inner
						for (int i = 0; i < param_count + 1; i++)
							pop(compiler->owner->current_module_record);
						compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
						return T_ANY;
					}
					param_types = grown;
				}
				param_types[param_count++] = inner;
			} while (match(compiler, CRUX_TOKEN_COMMA));
		}
		consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' to end function argument types.");

		consume(compiler, CRUX_TOKEN_ARROW, "Expected '->' to separate function argument types from return type.");
		ObjectTypeRecord *return_type = parse_type_record(compiler);
		if (!return_type) {
			for (int i = 0; i < param_count; i++) {
				pop(compiler->owner->current_module_record); // param_types[i]
			}
			compiler_panic(compiler->parser, "Expected type.", TYPE);
			FREE_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_capacity);
			return T_ANY;
		}
		push(compiler->owner->current_module_record, OBJECT_VAL(return_type));

		if (param_count < param_capacity) {
			param_types = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_capacity, param_count);
		}

		type_record = new_type_rec(compiler->owner, FUNCTION_TYPE);
		type_record->as.function_type.arg_types = param_types;
		type_record->as.function_type.arg_count = param_count;
		type_record->as.function_type.return_type = return_type;

		pop(compiler->owner->current_module_record); // return_type
		for (int i = 0; i < param_count; i++) {
			pop(compiler->owner->current_module_record); // param_types[i]
		}
	} else if (match(compiler, CRUX_TOKEN_RANDOM_TYPE)) {
		type_record = new_type_rec(compiler->owner, RANDOM_TYPE);
	} else if (match(compiler, CRUX_TOKEN_FILE_TYPE)) {
		type_record = new_type_rec(compiler->owner, FILE_TYPE);
	} else if (check(compiler, CRUX_TOKEN_IDENTIFIER)) {
		advance(compiler);
		ObjectString *name_str = copy_string(compiler->owner, compiler->parser->previous.start,
											 compiler->parser->previous.length);
		ObjectTypeRecord *found = NULL;
		Compiler *comp = compiler;
		while (comp != NULL) {
			if (type_table_get(comp->type_table, name_str, &found))
				break;
			comp = comp->enclosing;
		}
		if (!found) {
			compiler_panicf(compiler->parser, TYPE, "Unknown type: %s", name_str->chars);
			type_record = T_ANY;
		} else {
			type_record = found;
		}
	} else {
		compiler_panic(compiler->parser, "Expected type.", TYPE);
		type_record = T_ANY;
	}

	if (type_record && match(compiler, CRUX_TOKEN_PIPE)) {
		push(compiler->owner->current_module_record, OBJECT_VAL(type_record));
		int capacity = 4;
		int count = 1;
		ObjectTypeRecord **variants = ALLOCATE(compiler->owner, ObjectTypeRecord *, capacity);

		if (!variants) {
			compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
			pop(compiler->owner->current_module_record);
			return type_record;
		}
		variants[0] = type_record;

		do {
			if (count == capacity) {
				capacity = GROW_CAPACITY(capacity);
				ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, variants, count, capacity);
				if (!grown) {
					FREE_ARRAY(compiler->owner, ObjectTypeRecord *, variants, count);
					compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
					for (int i = 0; i < count; i++)
						pop(compiler->owner->current_module_record);
					return type_record;
				}
				variants = grown;
			}
			ObjectTypeRecord *variant = parse_type_record(compiler);
			push(compiler->owner->current_module_record, OBJECT_VAL(variant));
			variants[count++] = variant;
		} while (match(compiler, CRUX_TOKEN_PIPE));

		if (count < capacity) {
			variants = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, variants, capacity, count);
		}

		type_record = new_union_type_rec(compiler->owner, variants, NULL, count);

		for (int i = 0; i < count; i++) {
			pop(compiler->owner->current_module_record);
		}
	}

	return type_record;
}

bool init_compiler(CruxVM *vm, Compiler *compiler, Compiler *enclosing, const FunctionType type)
{
	if (vm == NULL)
		return false;
	if (compiler == NULL)
		return false;

	compiler->enclosed = NULL;
	compiler->enclosing = enclosing;

	if (enclosing) {
		enclosing->enclosed = compiler;
	}

	// if we have a parent compiler, and the type is not TYPE_SCRIPT, share the parser
	if (enclosing && type != TYPE_SCRIPT) {
		compiler->parser = enclosing->parser;
	} else { // otherwise, allocate a new parser
		compiler->parser = malloc(sizeof(Parser));
		if (compiler->parser == NULL)
			return false;
		compiler->parser->vm = vm;
		compiler->parser->source = NULL;
		compiler->parser->scanner = malloc(sizeof(Scanner));
		if (compiler->parser->scanner == NULL) {
			free(compiler->parser);
			return false;
		}
		compiler->parser->scanner->start = NULL;
		compiler->parser->scanner->current = NULL;
		compiler->parser->scanner->line = 1;

		compiler->parser->current = (Token){0};
		compiler->parser->previous = (Token){0};
		compiler->parser->had_error = false;
		compiler->parser->panic_mode = false;
	}

	memset(compiler->locals, 0, sizeof(compiler->locals));
	memset(compiler->upvalues, 0, sizeof(compiler->upvalues));
	memset(compiler->type_stack, 0, sizeof(compiler->type_stack));
	memset(compiler->loop_stack, 0, sizeof(compiler->loop_stack));

	compiler->type_stack_count = 0;
	compiler->type = type;
	compiler->local_count = 0;
	compiler->scope_depth = 0;
	compiler->match_depth = 0;
	compiler->loop_depth = 0;
	compiler->owner = vm;
	compiler->has_return = false;
	compiler->return_type = NULL;
	compiler->last_give_type = NULL;

	compiler->type_table = new_type_table(vm, INITIAL_TYPE_TABLE_SIZE);
	init_table(&compiler->globals);
	compiler->global_count = 0;

	// Copy indexes and types from previous module
	if (vm->current_module_record != NULL) {
		table_add_all(vm, &vm->current_module_record->global_names, &compiler->globals);
		compiler->global_count = (int)vm->current_module_record->global_count;
		type_table_add_all(vm->current_module_record->types, compiler->type_table);
	}

	compiler->function = new_function(compiler->owner);

	if (type == TYPE_ANONYMOUS) {
		compiler->function->name = copy_string(compiler->owner, "anonymous", 9);
	} else if (type != TYPE_SCRIPT) {
		compiler->function->name = copy_string(compiler->owner, compiler->parser->previous.start,
											   compiler->parser->previous.length);
	}

	// global initialization
	if (type == TYPE_SCRIPT) {
		if (vm->current_module_record != NULL) {
			table_add_all(vm, &vm->current_module_record->global_names, &compiler->globals);
			compiler->global_count = (int)vm->current_module_record->global_count;
			type_table_add_all(vm->current_module_record->types, compiler->type_table);
		}

		// set up core functions
		for (int i = 0; i < vm->core_fns.capacity; i++) {
			if (vm->core_fns.entries[i].key == NULL) {
				continue;
			}

			ObjectString *name = vm->core_fns.entries[i].key;
			CruxValue val = vm->core_fns.entries[i].value;

			// Generate types for the type_table
			if (IS_CRUX_NATIVE_CALLABLE(val)) {
				ObjectTypeRecord *existing_type;
				if (!type_table_get(compiler->type_table, name, &existing_type)) {
					const ObjectNativeCallable *callable = AS_CRUX_NATIVE_CALLABLE(val);
					ObjectTypeRecord **args_copy = NULL;
					if (callable->arity > 0) {
						args_copy = ALLOCATE(vm, ObjectTypeRecord *, callable->arity);
						for (int j = 0; j < callable->arity; j++) {
							args_copy[j] = callable->arg_types[j];
						}
					}
					ObjectTypeRecord *fn_type = new_function_type_rec(vm, args_copy, callable->arity,
																	  callable->return_type);
					type_table_set(compiler->type_table, name, fn_type);

					if (vm->current_module_record) {
						type_table_set(vm->current_module_record->types, name, fn_type);
					}
				}
			}

			// Check if it already has an index
			CruxValue existing_index;
			if (table_get(&compiler->globals, name, &existing_index)) {
				continue;
			}

			// Assign a new global index
			int global_index = compiler->global_count++;
			table_set(vm, &compiler->globals, name, INT_VAL(global_index));
			if (vm->current_module_record) {
				table_set(vm, &vm->current_module_record->global_names, name, INT_VAL(global_index));
			}

			uint16_t const_index = make_constant(compiler, val);
			emit_words(compiler, OP_CONSTANT, const_index);
			emit_words(compiler, OP_DEFINE_GLOBAL, global_index);
		}
	}

	Local *local = &compiler->locals[compiler->local_count++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
	local->is_captured = false;
	local->type = T_ANY;

	if (type == TYPE_METHOD) {
		local->name.start = "self";
		local->name.length = 4;
	}

	compiler->current_narrowing.tracked_local_index = -1;
	compiler->current_narrowing.tracked_global_name = NULL;
	compiler->current_narrowing.tracked_is_typeof = false;
	compiler->current_narrowing.tracked_literal_type = NULL;
	compiler->current_narrowing.local_index = -1;
	compiler->current_narrowing.global_name = NULL;
	compiler->current_narrowing.narrowed_to = NULL;
	compiler->current_narrowing.stripped_down = NULL;

	return true;
}

ObjectFunction *end_compiler(Compiler *compiler)
{
	emit_return(compiler);
	push_type_record(compiler, compiler->return_type);
	ObjectFunction *function = compiler->function;
#ifdef DEBUG_PRINT_CODE
	if (!compiler->parser->had_error) {
		disassemble_chunk(compiler->owner, current_chunk(compiler),
						  function->name != NULL ? function->name->chars : "<script>");
	}
#endif

	function->module_record = compiler->owner->current_module_record;
	if (compiler->enclosing) {
		compiler->enclosing->enclosed = NULL;
	}
	return function;
}

/**
 * Compile a source string into a function object.
 * Expects the compiler to be initialized.
 */
ObjectFunction *compile(CruxVM *vm, Compiler *compiler, Compiler *enclosing, char *source)
{
	// Pre-scan pass
	if (!init_compiler(vm, compiler, enclosing, TYPE_SCRIPT)) {
		return NULL;
	}

	// store previous jump buffer
	jmp_buf previous_jump_buffer;
	memcpy(previous_jump_buffer, compiler->parser->jump_buffer, sizeof(jmp_buf));

	if (setjmp(compiler->parser->jump_buffer) != 0) {
		free(compiler->parser->scanner);
		free(compiler->parser);
		memcpy(compiler->parser->jump_buffer, previous_jump_buffer, sizeof(jmp_buf));
		return NULL;
	}

	init_scanner(compiler->parser->scanner, source);

	compiler->parser->had_error = false;
	compiler->parser->panic_mode = false;
	compiler->parser->source = source;

	// Run pre-scan, merging into a temporary staging table.
	ObjectTypeTable *staging = new_type_table(vm, INITIAL_TYPE_TABLE_SIZE);
	push(vm->current_module_record, OBJECT_VAL(staging));
	pre_scan(compiler, source, staging);

	for (int i = 0; i < staging->capacity; i++) {
		const TypeEntry *entry = &staging->entries[i];
		if (entry->key == NULL)
			continue;
		type_table_set(compiler->type_table, entry->key, entry->value);
	}
	pop(vm->current_module_record); // staging table

	// Main compiler pass
	init_scanner(compiler->parser->scanner, source);
	compiler->parser->had_error = false;
	compiler->parser->panic_mode = false;
	compiler->parser->source = source;

	advance(compiler);

	while (!match(compiler, CRUX_TOKEN_EOF)) {
		declaration(compiler);
	}

	ObjectFunction *function = end_compiler(compiler);
	if (function != NULL) {
		function->module_record = vm->current_module_record;
		function->module_record->global_count = compiler->global_count;

		// Persist the names and indexes
		table_add_all(vm, &compiler->globals, &vm->current_module_record->global_names);

		// Persist the Types so the next REPL line knows what they are!
		type_table_add_all(compiler->type_table, vm->current_module_record->types);
	}

	bool had_error = compiler->parser->had_error;

	memcpy(compiler->parser->jump_buffer, previous_jump_buffer, sizeof(jmp_buf));
	free(compiler->parser->scanner);
	free(compiler->parser);
	free_table(vm, &compiler->globals);
	return had_error ? NULL : function;
}

void mark_compiler_roots(CruxVM *vm, const Compiler *compiler)
{
	if (compiler == NULL)
		return;
	const Compiler *current = compiler;
	while (current != NULL) {
		mark_object(vm, (CruxObject *)current->function);
		mark_object(vm, (CruxObject *)current->return_type);
		mark_object(vm, (CruxObject *)current->last_give_type);
		mark_object(vm, (CruxObject *)current->type_table);

		if (current->current_narrowing.tracked_global_name != NULL)
			mark_object(vm, (CruxObject *)current->current_narrowing.tracked_global_name);
		if (current->current_narrowing.tracked_literal_type != NULL)
			mark_object(vm, (CruxObject *)current->current_narrowing.tracked_literal_type);
		if (current->current_narrowing.global_name != NULL)
			mark_object(vm, (CruxObject *)current->current_narrowing.global_name);
		if (current->current_narrowing.narrowed_to != NULL)
			mark_object(vm, (CruxObject *)current->current_narrowing.narrowed_to);
		if (current->current_narrowing.stripped_down != NULL)
			mark_object(vm, (CruxObject *)current->current_narrowing.stripped_down);

		for (int i = 0; i < current->type_stack_count; i++) {
			mark_object(vm, (CruxObject *)current->type_stack[i]);
		}
		for (int i = 0; i < current->local_count; i++) {
			mark_object(vm, (CruxObject *)current->locals[i].type);
		}

		for (int i = 0; i < current->match_depth; i++) {
			for (int j = 0; j < current->match_compiler[i].exhaustiveness.matched_types_count; j++) {
				mark_object(vm, (CruxObject *)current->match_compiler[i].exhaustiveness.matched_types[j]);
			}
			for (int j = 0; j < current->match_compiler[i].pattern_count; j++) {
				mark_object(vm, (CruxObject *)current->match_compiler[i].patterns[j].type_produced);
			}
			mark_object(vm, (CruxObject *)current->match_compiler[i].exhaustiveness.resultant_type);
			mark_object(vm, (CruxObject *)current->match_compiler[i].exhaustiveness.target_type);
		}

		current = (Compiler *)current->enclosed;
	}
}
