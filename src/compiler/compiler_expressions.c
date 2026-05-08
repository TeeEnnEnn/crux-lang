#include "compiler/compiler_expressions.h"
#include <errno.h>
#include "common.h"
#include "compiler/compiler_core.h"
#include "compiler/compiler_functions.h"
#include "compiler/compiler_helpers.h"
#include "compiler/compiler_match.h"
#include "panic.h"
#include "scanner.h"
#include "type_system.h"

void and_(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;

	const ObjectTypeRecord *left_type = pop_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(left_type));

	if (left_type && left_type->base_type != BOOL_TYPE && left_type->base_type != ANY_TYPE) {
		char got[128];
		type_record_name(left_type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "Left operand of 'and' must be of type 'Bool', got '%s'.", got);
	}

	const NarrowingInfo left_narrowing = compiler->current_narrowing;

	// Reset tracking for the right side
	compiler->current_narrowing.tracked_local_index = -1;
	compiler->current_narrowing.tracked_is_typeof = false;
	compiler->current_narrowing.tracked_literal_type = NULL;
	compiler->current_narrowing.local_index = -1;
	compiler->current_narrowing.narrowed_to = NULL;
	compiler->current_narrowing.stripped_down = NULL;
	compiler->current_narrowing.tracked_global_name = NULL;
	compiler->current_narrowing.global_name = NULL;

	const int endJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
	emit_word(compiler, OP_POP);

	// If the left side narrowed a variable, apply it temporarily while compiling the right side
	ObjectTypeRecord *original_type = NULL;
	if (left_narrowing.narrowed_to) {
		if (left_narrowing.local_index != -1) {
			original_type = compiler->locals[left_narrowing.local_index].type;
			compiler->locals[left_narrowing.local_index].type = left_narrowing.narrowed_to;
		} else if (left_narrowing.global_name) {
			type_table_get(compiler->type_table, left_narrowing.global_name, &original_type);
			type_table_set(compiler->type_table, left_narrowing.global_name, left_narrowing.narrowed_to);
		}
	}

	push(compiler->owner->current_module_record, original_type ? OBJECT_VAL(original_type) : NIL_VAL);

	parse_precedence(compiler, PREC_AND);

	pop(compiler->owner->current_module_record); // original_type

	// Restore original type if temporarily narrowed it
	if (left_narrowing.narrowed_to) {
		if (left_narrowing.local_index != -1) {
			compiler->locals[left_narrowing.local_index].type = original_type;
		} else if (left_narrowing.global_name) {
			type_table_set(compiler->type_table, left_narrowing.global_name, original_type);
		}
	}

	const ObjectTypeRecord *right_type = pop_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(right_type));

	if (right_type && right_type->base_type != BOOL_TYPE && right_type->base_type != ANY_TYPE) {
		char got[128];
		type_record_name(right_type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "Right operand of 'and' must be of type 'Bool', got '%s'.", got);
	}

	patch_jump(compiler, endJump);

	push_type_record(compiler, T_BOOL);

	// for 'and' if the right side didn't narrow anything preserve the left side's narrowing
	if (compiler->current_narrowing.local_index == -1 && compiler->current_narrowing.global_name == NULL) {
		compiler->current_narrowing = left_narrowing;
	}

	pop(compiler->owner->current_module_record); // right_type
	pop(compiler->owner->current_module_record); // left_type
}

void or_(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;

	const ObjectTypeRecord *left_type = pop_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(left_type));

	if (left_type && left_type->base_type != BOOL_TYPE && left_type->base_type != ANY_TYPE) {
		char got[128];
		type_record_name(left_type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "Left operand of 'or' must be of type 'Bool', got '%s'.", got);
	}

	// For 'or' we can't guarantee either side is true in the 'then' block, so discard narrowing.
	compiler->current_narrowing.tracked_local_index = -1;
	compiler->current_narrowing.tracked_is_typeof = false;
	compiler->current_narrowing.tracked_literal_type = NULL;
	compiler->current_narrowing.local_index = -1;
	compiler->current_narrowing.narrowed_to = NULL;
	compiler->current_narrowing.stripped_down = NULL;
	compiler->current_narrowing.tracked_global_name = NULL;
	compiler->current_narrowing.global_name = NULL;

	const int elseJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
	const int endJump = emit_jump(compiler, OP_JUMP);
	patch_jump(compiler, elseJump);
	emit_word(compiler, OP_POP);
	parse_precedence(compiler, PREC_OR);

	ObjectTypeRecord *right_type = pop_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(right_type));

	if (right_type && right_type->base_type != BOOL_TYPE && right_type->base_type != ANY_TYPE) {
		char got[128];
		type_record_name(right_type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "Right operand of 'or' must be of type 'Bool', got '%s'.", got);
	}

	patch_jump(compiler, endJump);

	// Discard right side narrowing for 'or'
	compiler->current_narrowing.tracked_local_index = -1;
	compiler->current_narrowing.tracked_is_typeof = false;
	compiler->current_narrowing.tracked_literal_type = NULL;
	compiler->current_narrowing.local_index = -1;
	compiler->current_narrowing.narrowed_to = NULL;
	compiler->current_narrowing.stripped_down = NULL;
	compiler->current_narrowing.tracked_global_name = NULL;
	compiler->current_narrowing.global_name = NULL;

	push_type_record(compiler, T_BOOL);

	pop(compiler->owner->current_module_record); // right_type
	pop(compiler->owner->current_module_record); // left_type
}

void array_literal(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	uint16_t elementCount = 0;
	ObjectTypeRecord *element_type = NULL;

	push(compiler->owner->current_module_record, NIL_VAL);
	const int type_root_stack_index = (int)(compiler->owner->current_module_record->stack_top -
											compiler->owner->current_module_record->stack - 1);

	if (!match(compiler, CRUX_TOKEN_RIGHT_SQUARE)) {
		do {
			expression(compiler);
			ObjectTypeRecord *value_type = pop_type_record(compiler);

			if (!element_type) {
				element_type = value_type;
			} else if (element_type->base_type != ANY_TYPE && value_type && value_type->base_type != ANY_TYPE) {
				if (!types_compatible(element_type, value_type)) {
					if ((element_type->base_type == INT_TYPE && value_type->base_type == FLOAT_TYPE) ||
						(element_type->base_type == FLOAT_TYPE && value_type->base_type == INT_TYPE)) {
						element_type = T_FLOAT;
					} else {
						element_type = T_ANY;
					}
				}
			}

			compiler->owner->current_module_record->stack[type_root_stack_index] = element_type
																					   ? OBJECT_VAL(element_type)
																					   : NIL_VAL;

			if (elementCount >= UINT16_MAX) {
				compiler_panic(compiler->parser, "Too many elements in array literal.", COLLECTION_EXTENT);
			}
			elementCount++;
		} while (match(compiler, CRUX_TOKEN_COMMA));
		consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after array elements.");
	}

	if (!element_type) {
		element_type = T_ANY;
		compiler->owner->current_module_record->stack[type_root_stack_index] = OBJECT_VAL(element_type);
	}

	emit_word(compiler, OP_ARRAY);
	emit_word(compiler, elementCount);

	ObjectTypeRecord *array_type = new_array_type_rec(compiler->owner, element_type);
	push_type_record(compiler, array_type);

	pop(compiler->owner->current_module_record); // element_type
}

void tuple_literal(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	uint16_t elementCount = 0;
	int element_capacity = 4;
	ObjectTypeRecord **element_types = ALLOCATE(compiler->owner, ObjectTypeRecord *, element_capacity);

	if (element_types == NULL) {
		compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
		push_type_record(compiler, T_ANY);
		return;
	}

	if (!match(compiler, CRUX_TOKEN_RIGHT_SQUARE)) {
		do {
			expression(compiler);
			ObjectTypeRecord *value_type = pop_type_record(compiler);
			push(compiler->owner->current_module_record, value_type ? OBJECT_VAL(value_type) : NIL_VAL);

			if (elementCount == element_capacity) {
				const int old_capacity = element_capacity;
				element_capacity = GROW_CAPACITY(element_capacity);
				ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, element_types, old_capacity,
													  element_capacity);
				if (grown == NULL) {
					FREE_ARRAY(compiler->owner, ObjectTypeRecord *, element_types, old_capacity);
					for (uint16_t i = 0; i < elementCount; i++) {
						pop(compiler->owner->current_module_record);
					}
					compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
					push_type_record(compiler, T_ANY);
					return;
				}
				element_types = grown;
			}

			element_types[elementCount++] = value_type;

			if (elementCount >= UINT16_MAX) {
				compiler_panic(compiler->parser, "Too many elements in tuple literal.", COLLECTION_EXTENT);
			}
		} while (match(compiler, CRUX_TOKEN_COMMA));
		consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after tuple elements.");
	}

	if (elementCount < element_capacity) {
		ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, element_types, element_capacity,
											  elementCount);
		if (grown != NULL || elementCount == 0) {
			element_types = grown;
		}
	}

	emit_word(compiler, OP_TUPLE);
	emit_word(compiler, elementCount);

	ObjectTypeRecord *tuple_type = new_tuple_type_rec(compiler->owner, element_types, elementCount);
	push_type_record(compiler, tuple_type);

	for (uint16_t i = 0; i < elementCount; i++) {
		pop(compiler->owner->current_module_record);
	}
}

void table_literal(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	uint16_t elementCount = 0;
	ObjectTypeRecord *table_key_type = NULL;
	ObjectTypeRecord *table_value_type = NULL;

	push(compiler->owner->current_module_record, NIL_VAL); // key
	push(compiler->owner->current_module_record, NIL_VAL); // val
	const int val_idx = (int)(compiler->owner->current_module_record->stack_top -
							  compiler->owner->current_module_record->stack - 1);
	const int key_idx = val_idx - 1;

	if (!match(compiler, CRUX_TOKEN_RIGHT_BRACE)) {
		do {
			expression(compiler);
			ObjectTypeRecord *key_type = pop_type_record(compiler);
			consume(compiler, CRUX_TOKEN_COLON, "Expected ':' after table key.");

			push(compiler->owner->current_module_record, OBJECT_VAL(key_type));
			expression(compiler);
			ObjectTypeRecord *value_type = pop_type_record(compiler);
			pop(compiler->owner->current_module_record);

			if (!table_key_type) {
				table_key_type = key_type;
			} else if (table_key_type->base_type != ANY_TYPE && key_type && key_type->base_type != ANY_TYPE) {
				if (!types_equal(table_key_type, key_type)) {
					char expected[128], got[128];
					type_record_name(table_key_type, expected, sizeof(expected));
					type_record_name(key_type, got, sizeof(got));
					compiler_panicf(compiler->parser, TYPE,
									"Inconsistent key types in table literal: expected '%s', got '%s'.", expected, got);
				}
			}

			if (!table_value_type) {
				table_value_type = value_type;
			} else if (table_value_type->base_type != ANY_TYPE && value_type && value_type->base_type != ANY_TYPE) {
				if (!types_compatible(table_value_type, value_type)) {
					if ((table_value_type->base_type == INT_TYPE && value_type->base_type == FLOAT_TYPE) ||
						(table_value_type->base_type == FLOAT_TYPE && value_type->base_type == INT_TYPE)) {
						table_value_type = T_FLOAT;
					} else {
						table_value_type = T_ANY;
					}
				}
			}

			// update roots
			compiler->owner->current_module_record->stack[key_idx] = table_key_type ? OBJECT_VAL(table_key_type)
																					: NIL_VAL;
			compiler->owner->current_module_record->stack[val_idx] = table_value_type ? OBJECT_VAL(table_value_type)
																					  : NIL_VAL;

			if (elementCount >= UINT16_MAX) {
				compiler_panic(compiler->parser, "Too many elements in table literal.", COLLECTION_EXTENT);
			}
			elementCount++;
		} while (match(compiler, CRUX_TOKEN_COMMA));
		consume(compiler, CRUX_TOKEN_RIGHT_BRACE, "Expected '}' after table elements.");
	}

	if (!table_key_type)
		table_key_type = T_ANY;
	if (!table_value_type)
		table_value_type = T_ANY;
	compiler->owner->current_module_record->stack[key_idx] = OBJECT_VAL(table_key_type);
	compiler->owner->current_module_record->stack[val_idx] = OBJECT_VAL(table_value_type);

	emit_word(compiler, OP_TABLE);
	emit_word(compiler, elementCount);

	ObjectTypeRecord *table_type = new_table_type_rec(compiler->owner, table_key_type, table_value_type);
	push_type_record(compiler, table_type);

	pop(compiler->owner->current_module_record); // val
	pop(compiler->owner->current_module_record); // key
}

void collection_index(Compiler *compiler, const bool can_assign)
{
	ObjectTypeRecord *collection_type = pop_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(collection_type));

	expression(compiler);
	ObjectTypeRecord *index_type = pop_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(index_type));

	if (collection_type && index_type && index_type->base_type != ANY_TYPE && collection_type->base_type != ANY_TYPE) {
		if (collection_type->base_type == ARRAY_TYPE || collection_type->base_type == STRING_TYPE ||
			collection_type->base_type == TUPLE_TYPE || collection_type->base_type == BUFFER_TYPE) {
			if (index_type->base_type != INT_TYPE && index_type->base_type != RANGE_TYPE) {
				char got[128];
				type_record_name(index_type, got, sizeof(got));
				compiler_panicf(compiler->parser, TYPE, "Collection index must be of type 'Int' | 'Range', got '%s'.",
								got);
			}
		} else if (collection_type->base_type == TABLE_TYPE) {
			ObjectTypeRecord *key_type = collection_type->as.table_type.key_type;
			if (key_type && key_type->base_type != ANY_TYPE && !types_compatible(key_type, index_type)) {
				char expected[128], got[128];
				type_record_name(key_type, expected, sizeof(expected));
				type_record_name(index_type, got, sizeof(got));
				compiler_panicf(compiler->parser, TYPE, "Table key type mismatch: expected '%s', got '%s'.", expected,
								got);
			}
		}
	}

	bool is_slice = index_type && index_type->base_type == RANGE_TYPE;

	consume(compiler, CRUX_TOKEN_RIGHT_SQUARE, "Expected ']' after index.");

	if (can_assign && match(compiler, CRUX_TOKEN_EQUAL)) {
		if (is_slice) {
			compiler_panicf(compiler->parser, TYPE, "Cannot assign to a range slice of a collection.");
		}

		expression(compiler);
		ObjectTypeRecord *value_type = pop_type_record(compiler);

		if (collection_type && value_type && value_type->base_type != ANY_TYPE &&
			collection_type->base_type != ANY_TYPE) {
			ObjectTypeRecord *expected_value = NULL;
			if (collection_type->base_type == ARRAY_TYPE) {
				expected_value = collection_type->as.array_type.element_type;
			} else if (collection_type->base_type == TABLE_TYPE) {
				expected_value = collection_type->as.table_type.value_type;
			}
			if (expected_value && expected_value->base_type != ANY_TYPE &&
				!types_compatible(expected_value, value_type)) {
				char expected[128], got[128];
				type_record_name(expected_value, expected, sizeof(expected));
				type_record_name(value_type, got, sizeof(got));
				compiler_panicf(compiler->parser, TYPE, "Cannot assign '%s' to collection of element type '%s'.", got,
								expected);
			}
		}

		emit_word(compiler, OP_SET_COLLECTION);
		push_type_record(compiler, T_NIL);
	} else {
		if (is_slice) {
			emit_word(compiler, OP_GET_SLICE);
			ObjectTypeRecord *result_type = T_ANY;
			if (collection_type) {
				switch (collection_type->base_type) {
				case TABLE_TYPE: {
					compiler_panicf(compiler->parser, TYPE, "Cannot index a table with a range slice.");
					break;
				}
				case ARRAY_TYPE: {
					result_type = new_array_type_rec(compiler->owner, collection_type->as.array_type.element_type);
					break;
				}
				case STRING_TYPE: {
					result_type = T_STRING;
					break;
				}
				case TUPLE_TYPE: {
					result_type = new_tuple_type_rec(compiler->owner, NULL, -1);
					break;
				}
				case BUFFER_TYPE: {
					ObjectTypeRecord *element_type = T_INT;
					push(compiler->owner->current_module_record, OBJECT_VAL(element_type));
					result_type = new_array_type_rec(compiler->owner, element_type);
					pop(compiler->owner->current_module_record);
					break;
				}
				default: {
					break;
				}
				}
			}
			push_type_record(compiler, result_type);
		} else {
			emit_word(compiler, OP_GET_COLLECTION);
			ObjectTypeRecord *result_type = NULL;
			if (collection_type) {
				if (collection_type->base_type == ARRAY_TYPE) {
					result_type = collection_type->as.array_type.element_type;
				} else if (collection_type->base_type == TABLE_TYPE) {
					result_type = collection_type->as.table_type.value_type;
				} else if (collection_type->base_type == TUPLE_TYPE) {
					result_type = T_ANY; // cannot determine element type
				} else if (collection_type->base_type == STRING_TYPE) {
					result_type = T_STRING;
				} else if (collection_type->base_type == BUFFER_TYPE) {
					result_type = T_INT;
				}
			}
			push_type_record(compiler, result_type ? result_type : T_ANY);
		}
	}

	pop(compiler->owner->current_module_record); // index_type
	pop(compiler->owner->current_module_record); // collection_type
}

void result_unwrap(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;

	ObjectTypeRecord *type = pop_type_record(compiler);

	if (!type || type->base_type == ANY_TYPE) {
		// Unknown type — vm will catch errors.
		emit_word(compiler, OP_UNWRAP);
		push_type_record(compiler, T_ANY);
		return;
	}

	if (type->base_type != RESULT_TYPE) {
		char got[128];
		type_record_name(type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "'?' operator requires a 'Result' type, got '%s'.", got);
		push_type_record(compiler, T_ANY);
		return;
	}

	if (check_previous_op_code(compiler, OP_INVOKE_STDLIB, 3)) {
		set_previous_op_code(compiler, OP_INVOKE_STDLIB_UNWRAP, 3);
	} else {
		emit_word(compiler, OP_UNWRAP);
	}

	ObjectTypeRecord *ok_type = type->as.result_type.ok_type;
	push_type_record(compiler, ok_type ? ok_type : T_ANY);
}

static void binary_number(Compiler *compiler, bool can_assign)
{
	(void)can_assign;
	char *end = NULL;
	// +2 to skip the "0b" prefix
	long n = strtol(compiler->parser->previous.start + 2, &end, 2);
	if (end == compiler->parser->previous.start) {
		compiler_panic(compiler->parser, "Failed to parse binary literal.", SYNTAX);
		push_type_record(compiler, T_ANY);
		return;
	}
	emit_constant(compiler, INT_VAL((int32_t)n));
	push_type_record(compiler, T_INT);
}

static void hex_number(Compiler *compiler, bool can_assign)
{
	(void)can_assign;
	char *end = NULL;
	// +2 to skip the "0x" prefix
	long n = strtol(compiler->parser->previous.start + 2, &end, 16);
	if (end == compiler->parser->previous.start) {
		compiler_panic(compiler->parser, "Failed to parse hex literal.", SYNTAX);
		push_type_record(compiler, T_ANY);
		return;
	}
	emit_constant(compiler, INT_VAL((int32_t)n));
	push_type_record(compiler, T_INT);
}

void number(Compiler *compiler, bool can_assign)
{
	(void)can_assign;
	if (check(compiler, CRUX_TOKEN_DOT_DOT)) {
		if (compiler->parser->previous.type != CRUX_TOKEN_INT) {
			compiler_panic(compiler->parser, "Range literals require an Int start value.", TYPE);
			push_type_record(compiler, T_ANY);
			return;
		}

		char *start_end = NULL;
		long start_long = strtol(compiler->parser->previous.start, &start_end, 10);
		if (start_end == compiler->parser->previous.start) {
			compiler_panic(compiler->parser, "Failed to parse range start.", SYNTAX);
			push_type_record(compiler, T_ANY);
			return;
		}

		const int32_t start = (int32_t)start_long;
		int32_t step = 1;
		int32_t end_value = 0;

		advance(compiler); // consume '..'
		if (!parse_signed_int_literal(compiler, &end_value, "Expected Int literal after '..' in range literal.")) {
			push_type_record(compiler, T_ANY);
			return;
		}

		if (match(compiler, CRUX_TOKEN_DOT_DOT)) {
			step = end_value;
			if (!parse_signed_int_literal(compiler, &end_value,
										  "Expected Int literal after second '..' in range literal.")) {
				push_type_record(compiler, T_ANY);
				return;
			}
		}

		if (step == 0) {
			compiler_panic(compiler->parser, "Range literal step cannot be zero.", VALUE);
			push_type_record(compiler, T_ANY);
			return;
		}
		if (step > 0 && start > end_value) {
			compiler_panic(compiler->parser, "Range literal start cannot be greater than end when step is positive.",
						   VALUE);
			push_type_record(compiler, T_ANY);
			return;
		}
		if (step < 0 && start < end_value) {
			compiler_panic(compiler->parser, "Range literal start cannot be less than end when step is negative.",
						   VALUE);
			push_type_record(compiler, T_ANY);
			return;
		}

		emit_constant(compiler, INT_VAL(start));
		emit_constant(compiler, INT_VAL(step));
		emit_constant(compiler, INT_VAL(end_value));
		emit_word(compiler, OP_RANGE);
		push_type_record(compiler, new_type_rec(compiler->owner, RANGE_TYPE));
		return;
	}

	char *end;
	errno = 0;

	const char *numberStart = compiler->parser->previous.start;
	const double number = strtod(numberStart, &end);

	if (end == numberStart) {
		compiler_panic(compiler->parser, "Failed to form number", SYNTAX);
		return;
	}
	if (errno == ERANGE) {
		emit_constant(compiler, FLOAT_VAL(number));
		push_type_record(compiler, T_FLOAT);
		return;
	}
	bool hasDecimalNotation = false;
	for (const char *c = numberStart; c < end; c++) {
		if (*c == '.' || *c == 'e' || *c == 'E') {
			hasDecimalNotation = true;
			break;
		}
	}
	if (hasDecimalNotation) {
		if (number == 0.0) {
			emit_word(compiler, OP_0_FLOAT);
		} else if (number == 1.0) {
			emit_word(compiler, OP_1_FLOAT);
		} else if (number == 2.0) {
			emit_word(compiler, OP_2_FLOAT);
		} else {
			emit_constant(compiler, FLOAT_VAL(number));
		}
		push_type_record(compiler, T_FLOAT);
	} else {
		const int32_t integer = (int32_t)number;
		if ((double)integer == number) {
			if (number == 0.0) {
				emit_word(compiler, OP_0_INT);
			} else if (number == 1.0) {
				emit_word(compiler, OP_1_INT);
			} else if (number == 2.0) {
				emit_word(compiler, OP_2_INT);
			} else {
				emit_constant(compiler, INT_VAL(integer));
			}
			push_type_record(compiler, T_INT);
		} else {
			emit_constant(compiler, FLOAT_VAL(number));
			push_type_record(compiler, T_FLOAT);
		}
	}
}

static char process_escape_sequence(const char escape, bool *hasError)
{
	*hasError = false;
	switch (escape) {
	case 'n':
		return '\n';
	case 't':
		return '\t';
	case 'r':
		return '\r';
	case '\\':
		return '\\';
	case '"':
		return '"';
	case '\'':
		return '\'';
	case '0':
		return ' ';
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 'f':
		return '\f';
	case 'v':
		return '\v';
	default: {
		*hasError = true;
		return '\0';
	}
	}
}

void string(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	char *processed = ALLOCATE(compiler->owner, char, compiler->parser->previous.length);

	if (processed == NULL) {
		compiler_panic(compiler->parser, "Cannot allocate memory for string expression.", MEMORY);
		return;
	}

	int processedLength = 0;
	const char *src = (char *)compiler->parser->previous.start + 1;
	const int srcLength = compiler->parser->previous.length - 2;

	if (srcLength == 0) {
		ObjectString *string = copy_string(compiler->owner, "", 0);
		push_type_record(compiler, T_STRING);
		emit_constant(compiler, OBJECT_VAL(string));
		FREE_ARRAY(compiler->owner, char, processed, compiler->parser->previous.length);
		return;
	}

	for (int i = 0; i < srcLength; i++) {
		if (src[i] == '\\') {
			if (i + 1 >= srcLength) {
				compiler_panic(compiler->parser, "Unterminated escape sequence at end of string", SYNTAX);
				FREE_ARRAY(compiler->owner, char, processed, compiler->parser->previous.length);
				return;
			}

			bool error;
			const char escaped = process_escape_sequence(src[i + 1], &error);
			if (error) {
				compiler_panicf(compiler->parser, SYNTAX, "Unexpected escape sequence '\\%c'", src[i + 1]);
				FREE_ARRAY(compiler->owner, char, processed, compiler->parser->previous.length);
				return;
			}

			processed[processedLength++] = escaped;
			i++;
		} else {
			processed[processedLength++] = src[i];
		}
	}

	char *temp = GROW_ARRAY(compiler->owner, char, processed, compiler->parser->previous.length, processedLength + 1);
	if (temp == NULL) {
		compiler_panic(compiler->parser, "Cannot allocate memory for string expression.", MEMORY);
		FREE_ARRAY(compiler->owner, char, processed, compiler->parser->previous.length);
		return;
	}
	processed = temp;
	processed[processedLength] = '\0';
	ObjectString *string = take_string(compiler->owner, processed, processedLength);

	compiler->current_narrowing.tracked_literal_type = type_from_string(compiler->owner, compiler->type_table,
																		string->chars);

	push_type_record(compiler, T_STRING);
	emit_constant(compiler, OBJECT_VAL(string));
}

void unary(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	const CruxTokenType operatorType = compiler->parser->previous.type;

	// compile the operand
	parse_precedence(compiler, PREC_UNARY);

	switch (operatorType) {
	case CRUX_TOKEN_NOT: {
		// check if this is a boolean type
		ObjectTypeRecord *bool_expected = pop_type_record(compiler);
		if (!bool_expected || (bool_expected->base_type != ANY_TYPE && bool_expected->base_type != BOOL_TYPE)) {
			char got[128];
			type_record_name(bool_expected, got, sizeof(got));
			compiler_panicf(compiler->parser, TYPE, "Expected 'Bool' type for 'not' operator, got '%s'.", got);
		}
		push_type_record(compiler, bool_expected);
		emit_word(compiler, OP_NOT);
		break;
	}
	case CRUX_TOKEN_MINUS: {
		// check if this is a negatable type
		ObjectTypeRecord *num_expected = pop_type_record(compiler);
		if (!num_expected ||
			(num_expected->base_type != ANY_TYPE && !(num_expected->base_type & (INT_TYPE | FLOAT_TYPE)))) {
			char got[128];
			type_record_name(num_expected, got, sizeof(got));
			compiler_panicf(compiler->parser, TYPE, "Expected 'Int | Float' type for '-' operator, got '%s'.", got);
		}
		push_type_record(compiler, num_expected);
		emit_word(compiler, OP_NEGATE);
		break;
	}
	case CRUX_TOKEN_TILDE: {
		ObjectTypeRecord *int_expected = pop_type_record(compiler);
		if (!int_expected || (int_expected->base_type != ANY_TYPE && int_expected->base_type != INT_TYPE)) {
			compiler_panicf(compiler->parser, TYPE, "Expected 'Int' type for '~' operator.");
		}
		push_type_record(compiler, int_expected);
		emit_word(compiler, OP_BITWISE_NOT);
		break;
	}
	default:
		return; // unreachable
	}
}

void typeof_expression(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	parse_precedence(compiler, PREC_UNARY);
	compiler->current_narrowing.tracked_is_typeof = true;
	emit_word(compiler, OP_TYPEOF); // emits string representation of the type at runtime
	push_type_record(compiler, T_STRING);
}

/**
 * Ok(<expression>)
 */
void ok_expression(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	consume(compiler, CRUX_TOKEN_LEFT_PAREN, "Expected '(' after 'Ok' keyword.");
	expression(compiler);
	consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after 'Ok' value.");
	ObjectTypeRecord *type = new_type_rec(compiler->owner, RESULT_TYPE);
	type->as.result_type.ok_type = pop_type_record(compiler);
	emit_word(compiler, OP_OK);
	push_type_record(compiler, type);
}

/**
 * Err(<expression>)
 */
void err_expression(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	consume(compiler, CRUX_TOKEN_LEFT_PAREN, "Expected '(' after 'Err' keyword.");
	expression(compiler);
	consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after 'Err' value.");
	pop_type_record(compiler); // discard the type
	emit_word(compiler, OP_ERR);
	ObjectTypeRecord *any = T_ANY;
	push(compiler->owner->current_module_record, OBJECT_VAL(any));
	ObjectTypeRecord *type = new_type_rec(compiler->owner, RESULT_TYPE);
	type->as.result_type.ok_type = any;
	pop(compiler->owner->current_module_record);
	push_type_record(compiler, type);
}

/**
 * Some(<expression>)
 */
void some_expression(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	consume(compiler, CRUX_TOKEN_LEFT_PAREN, "Expected '(' after 'Some' keyword.");
	expression(compiler);
	consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after 'Some' value.");
	ObjectTypeRecord *type = new_type_rec(compiler->owner, OPTION_TYPE);
	type->as.option_type.some_type = pop_type_record(compiler);
	emit_word(compiler, OP_SOME);
	push_type_record(compiler, type);
}

/**
 * None
 */
void none_expression(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	emit_word(compiler, OP_NONE);
	// no type
	push_type_record(compiler, new_option_type_rec(compiler->owner, T_ANY));
}

void type_coerce(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	ObjectTypeRecord *got_type = pop_type_record(compiler);
	ObjectTypeRecord *type_record = parse_type_record(compiler);

	if (got_type && type_record && got_type->base_type != ANY_TYPE && type_record->base_type != ANY_TYPE) {
		if (!types_compatible(type_record, got_type) && !types_compatible(got_type, type_record)) {
			char expected[128], got[128];
			type_record_name(type_record, expected, sizeof(expected));
			type_record_name(got_type, got, sizeof(got));
			compiler_panicf(compiler->parser, TYPE, "Invalid cast: cannot cast '%s' to '%s'.", got, expected);
		}
	}

	push_type_record(compiler, type_record);
	const uint16_t type_const = make_constant(compiler, OBJECT_VAL(type_record));
	emit_words(compiler, OP_TYPE_COERCE, type_const);
}

void named_variable(Compiler *compiler, Token name, const bool can_assign)
{
	ObjectString *name_str = copy_string(compiler->owner, name.start, name.length);
	push(compiler->owner->current_module_record, OBJECT_VAL(name_str));

	uint16_t getOp, setOp;
	int arg = resolve_local(compiler, &name);
	ObjectTypeRecord *var_type = NULL;
	int global_index = -1;

	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
		var_type = compiler->locals[arg].type;
	} else if ((arg = resolve_upvalue(compiler, &name)) != -1) {
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
		var_type = compiler->upvalues[arg].type;
	} else {
		arg = identifier_constant(compiler, &name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
		const Compiler *comp = compiler;
		while (comp != NULL) {
			if (var_type == NULL) {
				type_table_get(comp->type_table, name_str, &var_type);
			}
			if (global_index == -1) {
				CruxValue index_val;
				if (table_get(&comp->globals, name_str, &index_val)) {
					global_index = AS_INT(index_val);
					arg = global_index;
				}
			}
			if (var_type != NULL && global_index != -1) {
				break;
			}
			comp = comp->enclosing;
		}

		if (!var_type) {
			compiler_panicf(compiler->parser, TYPE, "Undeclared variable '%s'. Did you forget to declare or import it?",
							name_str->chars);
			var_type = T_ANY;
		}

		if (global_index == -1) {
			compiler_panicf(compiler->parser, TYPE, "Failed to get index for global variable '%s'.", name_str->chars);
		}
	}
	push(compiler->owner->current_module_record, OBJECT_VAL(var_type));

	if (can_assign) {
		if (match(compiler, CRUX_TOKEN_EQUAL)) {
			expression(compiler);
			ObjectTypeRecord *value_type = pop_type_record(compiler);

			if (var_type && value_type && var_type->base_type != ANY_TYPE && value_type->base_type != ANY_TYPE) {
				if (!types_compatible(var_type, value_type)) {
					char exp[128], got[128];
					type_record_name(var_type, exp, sizeof(exp));
					type_record_name(value_type, got, sizeof(got));
					compiler_panicf(compiler->parser, TYPE, "Cannot assign '%s' to variable of type '%s'.", got, exp);
				}
			}
			emit_words(compiler, setOp, arg);
			push_type_record(compiler, T_NIL);

			pop(compiler->owner->current_module_record); // var_type
			pop(compiler->owner->current_module_record); // name_str
			return;
		}

		const int op = match_compound_op(compiler);
		if (op != -1) {
			expression(compiler);
			ObjectTypeRecord *rhs_type = pop_type_record(compiler);
			check_compound_type_math(compiler, var_type, rhs_type, op);

			emit_words(compiler, get_compound_opcode(compiler, setOp, op), arg);
			push_type_record(compiler, T_NIL);

			pop(compiler->owner->current_module_record); // var_type
			pop(compiler->owner->current_module_record); // name_str
			return;
		}
	}

	if (getOp == OP_GET_LOCAL) {
		compiler->current_narrowing.tracked_local_index = arg;
		compiler->current_narrowing.tracked_global_name = NULL;
	} else if (getOp == OP_GET_GLOBAL) {
		compiler->current_narrowing.tracked_global_name = name_str;
		compiler->current_narrowing.tracked_local_index = -1;
	} else {
		compiler->current_narrowing.tracked_local_index = -1;
		compiler->current_narrowing.tracked_global_name = NULL;
	}

	emit_words(compiler, getOp, arg);
	push_type_record(compiler, var_type);

	pop(compiler->owner->current_module_record); // var_type
	pop(compiler->owner->current_module_record); // name_str
}

void binary(Compiler *compiler, bool can_assign)
{
	(void)can_assign;
	const CruxTokenType operatorType = compiler->parser->previous.type;
	const ParseRule *rule = get_rule(operatorType);
	parse_precedence(compiler, rule->precedence + 1);

	ObjectTypeRecord *right_type = pop_type_record(compiler);
	ObjectTypeRecord *left_type = pop_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(left_type));
	push(compiler->owner->current_module_record, OBJECT_VAL(right_type));

	ObjectTypeRecord *result_type = NULL;
	push(compiler->owner->current_module_record, NIL_VAL);
	CruxValue *result_slot = compiler->owner->current_module_record->stack_top - 1;

	const bool either_any = (left_type && left_type->base_type == ANY_TYPE) ||
							(right_type && right_type->base_type == ANY_TYPE) || !left_type || !right_type;

	switch (operatorType) {
	case CRUX_TOKEN_EQUAL_EQUAL:
	case CRUX_TOKEN_BANG_EQUAL:
		if (operatorType == CRUX_TOKEN_EQUAL_EQUAL) {
			emit_word(compiler, OP_EQUAL);
		} else {
			emit_word(compiler, OP_NOT_EQUAL);
		}

		result_type = T_BOOL;
		*result_slot = OBJECT_VAL(result_type);

		if ((compiler->current_narrowing.tracked_local_index != -1 ||
			 compiler->current_narrowing.tracked_global_name != NULL) &&
			compiler->current_narrowing.tracked_literal_type != NULL) {
			compiler->current_narrowing.local_index = compiler->current_narrowing.tracked_local_index;
			compiler->current_narrowing.global_name = compiler->current_narrowing.tracked_global_name;

			ObjectTypeRecord *var_type = NULL;
			if (compiler->current_narrowing.local_index != -1) {
				var_type = compiler->locals[compiler->current_narrowing.local_index].type;
			} else {
				type_table_get(compiler->type_table, compiler->current_narrowing.global_name, &var_type);
			}

			ObjectTypeRecord *narrow = NULL;
			ObjectTypeRecord *stripped = NULL;

			if (compiler->current_narrowing.tracked_is_typeof ||
				compiler->current_narrowing.tracked_literal_type->base_type == NIL_TYPE) {
				narrow = compiler->current_narrowing.tracked_literal_type;
				stripped = strip_type(compiler->owner, var_type, narrow);
			}

			if (operatorType == CRUX_TOKEN_EQUAL_EQUAL) {
				compiler->current_narrowing.narrowed_to = narrow;
				compiler->current_narrowing.stripped_down = stripped;
			} else {
				compiler->current_narrowing.narrowed_to = stripped;
				compiler->current_narrowing.stripped_down = narrow;
			}
		}
		break;

	case CRUX_TOKEN_GREATER:
	case CRUX_TOKEN_GREATER_EQUAL:
	case CRUX_TOKEN_LESS:
	case CRUX_TOKEN_LESS_EQUAL: {
		if (!either_any) {
			const bool left_num = is_numeric_type(left_type);
			const bool right_num = is_numeric_type(right_type);

			if (!(left_num && right_num)) {
				compiler_panic(compiler->parser, "Comparison operator requires numeric operands.", TYPE);
			}
		}

		switch (operatorType) {
		case CRUX_TOKEN_GREATER:
			emit_word(compiler, OP_GREATER);
			break;
		case CRUX_TOKEN_GREATER_EQUAL:
			emit_word(compiler, OP_GREATER_EQUAL);
			break;
		case CRUX_TOKEN_LESS:
			emit_word(compiler, OP_LESS);
			break;
		case CRUX_TOKEN_LESS_EQUAL:
			emit_word(compiler, OP_LESS_EQUAL);
			break;
		default:
			break;
		}

		result_type = T_BOOL;
		break;
	}

	case CRUX_TOKEN_PLUS: {
		if (either_any) {
			emit_word(compiler, OP_ADD);
			result_type = T_ANY;
			break;
		}

		if (left_type->base_type == STRING_TYPE || right_type->base_type == STRING_TYPE) {
			if (left_type->base_type != STRING_TYPE || right_type->base_type != STRING_TYPE) {
				compiler_panic(compiler->parser, "Cannot use '+' between String and non-String.", TYPE);
			}
			emit_word(compiler, OP_ADD);
			result_type = T_STRING;
			break;
		}

		if (is_primitive_numeric_type(left_type) && is_primitive_numeric_type(right_type)) {
			emit_word(compiler,
					  left_type->base_type == INT_TYPE && right_type->base_type == INT_TYPE ? OP_ADD_INT : OP_ADD_NUM);
			result_type = (left_type->base_type == INT_TYPE && right_type->base_type == INT_TYPE) ? T_INT : T_FLOAT;
			break;
		}

		if (left_type->base_type == VECTOR_TYPE && right_type->base_type == VECTOR_TYPE) {
			emit_word(compiler, OP_ADD_VECTOR_VECTOR);
			result_type = new_vector_type_rec(compiler->owner,
											  merge_vector_dimensions(compiler, left_type, right_type, "addition"));
			break;
		}

		if (left_type->base_type == COMPLEX_TYPE && right_type->base_type == COMPLEX_TYPE) {
			emit_word(compiler, OP_ADD_COMPLEX_COMPLEX);
			result_type = new_type_rec(compiler->owner, COMPLEX_TYPE);
			break;
		}

		if (left_type->base_type == MATRIX_TYPE && right_type->base_type == MATRIX_TYPE) {
			emit_word(compiler, OP_ADD_MATRIX_MATRIX);
			result_type = merge_matrix_shape(compiler, left_type, right_type, "addition");
			break;
		}

		if (left_type->base_type == MATRIX_TYPE && is_primitive_numeric_type(right_type)) {
			emit_word(compiler, OP_ADD_MATRIX_SCALAR);
			result_type = new_matrix_type_rec(compiler->owner, left_type->as.matrix_type.rows,
											  left_type->as.matrix_type.cols);
			break;
		}

		if (is_primitive_numeric_type(left_type) && right_type->base_type == MATRIX_TYPE) {
			emit_word(compiler, OP_ADD_SCALAR_MATRIX);
			result_type = new_matrix_type_rec(compiler->owner, right_type->as.matrix_type.rows,
											  right_type->as.matrix_type.cols);
			break;
		}

		compiler_panic(compiler->parser, "'+' requires String, numeric, Vector, Complex, or Matrix operands.", TYPE);
		result_type = T_ANY;
		break;
	}

	case CRUX_TOKEN_MINUS:
	case CRUX_TOKEN_STAR: {
		if (either_any) {
			emit_word(compiler, operatorType == CRUX_TOKEN_MINUS ? OP_SUBTRACT : OP_MULTIPLY);
			result_type = T_ANY;
			break;
		}

		if (is_primitive_numeric_type(left_type) && is_primitive_numeric_type(right_type)) {
			if (operatorType == CRUX_TOKEN_MINUS) {
				emit_word(compiler, left_type->base_type == INT_TYPE && right_type->base_type == INT_TYPE
										? OP_SUBTRACT_INT
										: OP_SUBTRACT_NUM);
			} else {
				emit_word(compiler, left_type->base_type == INT_TYPE && right_type->base_type == INT_TYPE
										? OP_MULTIPLY_INT
										: OP_MULTIPLY_NUM);
			}
			result_type = (left_type->base_type == INT_TYPE && right_type->base_type == INT_TYPE) ? T_INT : T_FLOAT;
			break;
		}

		if (operatorType == CRUX_TOKEN_MINUS) {
			if (left_type->base_type == VECTOR_TYPE && right_type->base_type == VECTOR_TYPE) {
				emit_word(compiler, OP_SUBTRACT_VECTOR_VECTOR);
				result_type = new_vector_type_rec(compiler->owner, merge_vector_dimensions(compiler, left_type,
																						   right_type, "subtraction"));
				break;
			}
			if (left_type->base_type == COMPLEX_TYPE && right_type->base_type == COMPLEX_TYPE) {
				emit_word(compiler, OP_SUBTRACT_COMPLEX_COMPLEX);
				result_type = new_type_rec(compiler->owner, COMPLEX_TYPE);
				break;
			}
			if (left_type->base_type == MATRIX_TYPE && right_type->base_type == MATRIX_TYPE) {
				emit_word(compiler, OP_SUBTRACT_MATRIX_MATRIX);
				result_type = merge_matrix_shape(compiler, left_type, right_type, "subtraction");
				break;
			}
			if (left_type->base_type == MATRIX_TYPE && is_primitive_numeric_type(right_type)) {
				emit_word(compiler, OP_SUBTRACT_MATRIX_SCALAR);
				result_type = new_matrix_type_rec(compiler->owner, left_type->as.matrix_type.rows,
												  left_type->as.matrix_type.cols);
				break;
			}
			if (is_primitive_numeric_type(left_type) && right_type->base_type == MATRIX_TYPE) {
				emit_word(compiler, OP_SUBTRACT_SCALAR_MATRIX);
				result_type = new_matrix_type_rec(compiler->owner, right_type->as.matrix_type.rows,
												  right_type->as.matrix_type.cols);
				break;
			}
		} else {
			if (left_type->base_type == VECTOR_TYPE && right_type->base_type == VECTOR_TYPE) {
				const int left_dim = left_type->as.vector_type.dimensions;
				const int right_dim = right_type->as.vector_type.dimensions;
				if ((left_dim != -1 && left_dim != 3) || (right_dim != -1 && right_dim != 3)) {
					compiler_panic(compiler->parser, "Vector cross product requires 3D vectors.", TYPE);
				}
				emit_word(compiler, OP_MULTIPLY_VECTOR_VECTOR);
				result_type = new_vector_type_rec(compiler->owner, 3);
				break;
			}
			if (left_type->base_type == VECTOR_TYPE && is_primitive_numeric_type(right_type)) {
				emit_word(compiler, OP_MULTIPLY_VECTOR_SCALAR);
				result_type = new_vector_type_rec(compiler->owner, left_type->as.vector_type.dimensions);
				break;
			}
			if (is_primitive_numeric_type(left_type) && right_type->base_type == VECTOR_TYPE) {
				emit_word(compiler, OP_MULTIPLY_SCALAR_VECTOR);
				result_type = new_vector_type_rec(compiler->owner, right_type->as.vector_type.dimensions);
				break;
			}
			if (left_type->base_type == COMPLEX_TYPE && right_type->base_type == COMPLEX_TYPE) {
				emit_word(compiler, OP_MULTIPLY_COMPLEX_COMPLEX);
				result_type = new_type_rec(compiler->owner, COMPLEX_TYPE);
				break;
			}
			if (left_type->base_type == COMPLEX_TYPE && is_primitive_numeric_type(right_type)) {
				emit_word(compiler, OP_MULTIPLY_COMPLEX_SCALAR);
				result_type = new_type_rec(compiler->owner, COMPLEX_TYPE);
				break;
			}
			if (is_primitive_numeric_type(left_type) && right_type->base_type == COMPLEX_TYPE) {
				emit_word(compiler, OP_MULTIPLY_SCALAR_COMPLEX);
				result_type = new_type_rec(compiler->owner, COMPLEX_TYPE);
				break;
			}
			if (left_type->base_type == MATRIX_TYPE && right_type->base_type == MATRIX_TYPE) {
				emit_word(compiler, OP_MULTIPLY_MATRIX_MATRIX);
				result_type = matrix_multiply_result_type(compiler, left_type, right_type);
				break;
			}
			if (left_type->base_type == MATRIX_TYPE && is_primitive_numeric_type(right_type)) {
				emit_word(compiler, OP_MULTIPLY_MATRIX_SCALAR);
				result_type = new_matrix_type_rec(compiler->owner, left_type->as.matrix_type.rows,
												  left_type->as.matrix_type.cols);
				break;
			}
			if (is_primitive_numeric_type(left_type) && right_type->base_type == MATRIX_TYPE) {
				emit_word(compiler, OP_MULTIPLY_SCALAR_MATRIX);
				result_type = new_matrix_type_rec(compiler->owner, right_type->as.matrix_type.rows,
												  right_type->as.matrix_type.cols);
				break;
			}
		}

		{
			char left_name[128], right_name[128];
			type_record_name(left_type, left_name, sizeof(left_name));
			type_record_name(right_type, right_name, sizeof(right_name));
			compiler_panicf(compiler->parser, TYPE, "%s is not defined for '%s' and '%s'.",
							operatorType == CRUX_TOKEN_MINUS ? "'-'" : "'*'", left_name, right_name);
		}
		result_type = T_ANY;
		break;
	}

	case CRUX_TOKEN_SLASH: {
		if (either_any) {
			emit_word(compiler, OP_DIVIDE);
			result_type = T_FLOAT;
			break;
		}

		if (is_primitive_numeric_type(left_type) && is_primitive_numeric_type(right_type)) {
			emit_word(compiler, OP_DIVIDE_NUM);
			result_type = T_FLOAT;
			break;
		}
		if (left_type->base_type == VECTOR_TYPE && right_type->base_type == VECTOR_TYPE) {
			emit_word(compiler, OP_DIVIDE_VECTOR_VECTOR);
			result_type = new_vector_type_rec(compiler->owner,
											  merge_vector_dimensions(compiler, left_type, right_type, "division"));
			break;
		}
		if (left_type->base_type == VECTOR_TYPE && is_primitive_numeric_type(right_type)) {
			emit_word(compiler, OP_DIVIDE_VECTOR_SCALAR);
			result_type = new_vector_type_rec(compiler->owner, left_type->as.vector_type.dimensions);
			break;
		}
		if (left_type->base_type == COMPLEX_TYPE && right_type->base_type == COMPLEX_TYPE) {
			emit_word(compiler, OP_DIVIDE_COMPLEX_COMPLEX);
			result_type = new_type_rec(compiler->owner, COMPLEX_TYPE);
			break;
		}
		if (left_type->base_type == COMPLEX_TYPE && is_primitive_numeric_type(right_type)) {
			emit_word(compiler, OP_DIVIDE_COMPLEX_SCALAR);
			result_type = new_type_rec(compiler->owner, COMPLEX_TYPE);
			break;
		}
		if (left_type->base_type == MATRIX_TYPE && is_primitive_numeric_type(right_type)) {
			emit_word(compiler, OP_DIVIDE_MATRIX_SCALAR);
			result_type = new_matrix_type_rec(compiler->owner, left_type->as.matrix_type.rows,
											  left_type->as.matrix_type.cols);
			break;
		}

		{
			char left_name[128], right_name[128];
			type_record_name(left_type, left_name, sizeof(left_name));
			type_record_name(right_type, right_name, sizeof(right_name));
			compiler_panicf(compiler->parser, TYPE, "'/' is not defined for '%s' and '%s'.", left_name, right_name);
		}
		result_type = T_ANY;
		break;
	}

	case CRUX_TOKEN_PERCENT:
	case CRUX_TOKEN_BACKSLASH: {
		if (!either_any) {
			if (left_type->base_type != INT_TYPE || right_type->base_type != INT_TYPE) {
				char left_name[128], right_name[128];
				type_record_name(left_type, left_name, sizeof(left_name));
				type_record_name(right_type, right_name, sizeof(right_name));
				compiler_panicf(compiler->parser, TYPE, "%s requires Int operands, got '%s' and '%s'.",
								operatorType == CRUX_TOKEN_PERCENT ? "'%'" : "'\\'", left_name, right_name);
			}
		}
		if (either_any) {
			emit_word(compiler, operatorType == CRUX_TOKEN_PERCENT ? OP_MODULUS : OP_INT_DIVIDE);
		} else {
			emit_word(compiler, operatorType == CRUX_TOKEN_PERCENT ? OP_MODULUS_INT : OP_INT_DIVIDE_INT);
		}
		result_type = T_INT;
		break;
	}

	case CRUX_TOKEN_RIGHT_SHIFT:
	case CRUX_TOKEN_LEFT_SHIFT:
	case CRUX_TOKEN_AMPERSAND:
	case CRUX_TOKEN_CARET:
	case CRUX_TOKEN_PIPE: {
		if (!either_any) {
			if (left_type->base_type != INT_TYPE || right_type->base_type != INT_TYPE) {
				compiler_panic(compiler->parser, "Bitwise operators require Int operands.", TYPE);
			}
		}

		switch (operatorType) {
		case CRUX_TOKEN_RIGHT_SHIFT:
			emit_word(compiler, OP_RIGHT_SHIFT);
			break;
		case CRUX_TOKEN_LEFT_SHIFT:
			emit_word(compiler, OP_LEFT_SHIFT);
			break;
		case CRUX_TOKEN_AMPERSAND:
			emit_word(compiler, OP_BITWISE_AND);
			break;
		case CRUX_TOKEN_CARET:
			emit_word(compiler, OP_BITWISE_XOR);
			break;
		case CRUX_TOKEN_PIPE:
			emit_word(compiler, OP_BITWISE_OR);
			break;
		default:
			break;
		}

		result_type = T_INT;
		break;
	}

	case CRUX_TOKEN_STAR_STAR: {
		if (!either_any) {
			const bool left_num = left_type->base_type == INT_TYPE || left_type->base_type == FLOAT_TYPE;
			const bool right_num = right_type->base_type == INT_TYPE || right_type->base_type == FLOAT_TYPE;
			if (!left_num || !right_num) {
				char left_name[128], right_name[128];
				type_record_name(left_type, left_name, sizeof(left_name));
				type_record_name(right_type, right_name, sizeof(right_name));
				compiler_panicf(compiler->parser, TYPE, "'**' requires numeric operands, got '%s' and '%s'.", left_name,
								right_name);
			}
		}
		if (either_any) {
			emit_word(compiler, OP_POWER);
		} else {
			emit_word(compiler, left_type->base_type == INT_TYPE && right_type->base_type == INT_TYPE ? OP_POWER_INT
																									  : OP_POWER_NUM);
		}
		result_type = T_FLOAT;
		break;
	}

	case CRUX_TOKEN_IN: {
		if (!either_any) {
			if (!is_collection_type(right_type)) {
				char right_name[128];
				type_record_name(right_type, right_name, sizeof(right_name));
				compiler_panicf(compiler->parser, TYPE, "'in' requires a collection type, got '%s'.", right_name);
			}
		}
		emit_word(compiler, OP_IN);
		result_type = T_BOOL;
		break;
	}

	default:
		result_type = T_ANY;
		break;
	}

	// Update the slot
	*result_slot = result_type ? OBJECT_VAL(result_type) : NIL_VAL;
	push_type_record(compiler, result_type);

	pop(compiler->owner->current_module_record); // result_slot
	pop(compiler->owner->current_module_record); // right_type
	pop(compiler->owner->current_module_record); // left_type
}

void colon_colon(Compiler *compiler, const bool can_assign)
{
	(void) can_assign;
	consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected property name after '::'.");
	const uint16_t name_constant = identifier_constant(compiler, &compiler->parser->previous);
	const Token method_name_token = compiler->parser->previous;

	if (name_constant >= UINT16_MAX) {
		compiler_panic(compiler->parser, "Too many constants.", SYNTAX);
	}

	ObjectTypeRecord *object_type = peek_type_record(compiler);
	if (!object_type) {
		object_type = T_ANY;
	}
	push(compiler->owner->current_module_record, OBJECT_VAL(object_type));

	if (match(compiler, CRUX_TOKEN_LEFT_PAREN)) {
		uint16_t arg_count = 0;
		ObjectTypeRecord *arg_types[UINT8_COUNT] = {0};

		// compiling arguments
		if (!check(compiler, CRUX_TOKEN_RIGHT_PAREN)) {
			do {
				if (arg_count >= UINT8_COUNT) {
					for (int i = 0; i < arg_count; i++)
						pop(compiler->owner->current_module_record);
					pop(compiler->owner->current_module_record); // object_type
					compiler_panic(compiler->parser, "Cannot have more than 255 arguments.", ARGUMENT_EXTENT);
					return;
				}

				expression(compiler);
				arg_types[arg_count] = pop_type_record(compiler);
				push(compiler->owner->current_module_record, OBJECT_VAL(arg_types[arg_count]));
				arg_count++;
			} while (match(compiler, CRUX_TOKEN_COMMA));
		}
		consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");

		ObjectTypeRecord **method_arg_types = NULL;
		ObjectTypeRecord *method_return = NULL;
		int method_arity = 0;
		bool method_found = false;

		if (object_type->base_type == STRUCT_TYPE) {
			const ObjectTypeTable *field_types = object_type->as.struct_type.field_types;
			const ObjectString *field_name = copy_string(compiler->owner, method_name_token.start,
														 method_name_token.length);
			ObjectTypeRecord *fn_type = NULL;
			if (type_table_get(field_types, field_name, &fn_type) && fn_type && fn_type->base_type == FUNCTION_TYPE && fn_type->as.function_type.is_static) {
				method_arg_types = fn_type->as.function_type.arg_types;
				method_arity = fn_type->as.function_type.arg_count;
				method_return = fn_type->as.function_type.return_type;
				method_found = true;
			} else {
				compiler_panicf(compiler->parser, TYPE, "'%.*s' is not callable as a static method.",
								(int)method_name_token.length, method_name_token.start);
			}
		}

		if (method_found && method_arg_types) {
			const int param_offset = 0;
			int user_params = method_arity - param_offset;
			if (user_params < 0)
				user_params = 0;

			if ((int)arg_count != user_params) {
				compiler_panicf(compiler->parser, ARGUMENT_MISMATCH, "Method '%.*s' expects %d argument(s), got %d.",
								(int)method_name_token.length, method_name_token.start, user_params, (int)arg_count);
			} else {
				for (int i = 0; i < (int)arg_count; i++) {
					ObjectTypeRecord *expected = method_arg_types[i + param_offset];
					ObjectTypeRecord *got_type = arg_types[i];
					if (expected && got_type && expected->base_type != ANY_TYPE && got_type->base_type != ANY_TYPE &&
						!types_compatible(expected, got_type)) {
						char exp_name[128], got_name[128];
						type_record_name(expected, exp_name, sizeof(exp_name));
						type_record_name(got_type, got_name, sizeof(got_name));
						compiler_panicf(compiler->parser, TYPE, "Argument %d type mismatch: expected '%s', got '%s'.",
										i + 1, exp_name, got_name);
					}
				}
			}
		}

		emit_words(compiler, OP_STATIC_INVOKE, name_constant);
		emit_word(compiler, arg_count);

		pop_type_record(compiler);
		push_type_record(compiler, method_return ? method_return : T_ANY);

		for (int i = 0; i < (int)arg_count; i++) {
			pop(compiler->owner->current_module_record);
		}
		pop(compiler->owner->current_module_record); // object_type
		return;
	}
}

void dot(Compiler *compiler, const bool can_assign)
{
	const ObjectNativeCallable *stdlib_callable = NULL;
	consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected property name after '.'.");
	const uint16_t name_constant = identifier_constant(compiler, &compiler->parser->previous);
	const Token method_name_token = compiler->parser->previous;

	if (name_constant >= UINT16_MAX) {
		compiler_panic(compiler->parser, "Too many constants.", SYNTAX);
	}

	ObjectTypeRecord *object_type = peek_type_record(compiler);
	if (!object_type) {
		object_type = T_ANY;
	}
	push(compiler->owner->current_module_record, OBJECT_VAL(object_type));

	// Determine if we can use indexed access
	int field_index = -1;
	if (object_type->base_type == STRUCT_TYPE) {
		ObjectString *field_name = AS_CRUX_STRING(current_chunk(compiler)->constants.values[name_constant]);
		CruxValue index_val;
		if (table_get(&object_type->as.struct_type.definition->fields, field_name, &index_val)) {
			field_index = AS_INT(index_val);
		}
	}

	// OP_SET_PROPERTY - this only works for structs
	if (can_assign) {
		const ObjectString *field_name = copy_string(compiler->owner, method_name_token.start,
													 method_name_token.length);
		push(compiler->owner->current_module_record, OBJECT_VAL(field_name));
		ObjectTypeRecord *field_type = NULL;

		if (object_type->base_type == STRUCT_TYPE) {
			const ObjectTypeTable *field_types = object_type->as.struct_type.field_types;
			type_table_get(field_types, field_name, &field_type);
		}

		if (match(compiler, CRUX_TOKEN_EQUAL)) {
			expression(compiler);
			ObjectTypeRecord *value_type = pop_type_record(compiler);

			if (object_type->base_type == STRUCT_TYPE && !field_type) {
				compiler_panicf(compiler->parser, NAME, "Struct has no field '%.*s'.", (int)method_name_token.length,
								method_name_token.start);
			}

			if (field_type && field_type->base_type != ANY_TYPE && value_type->base_type != ANY_TYPE) {
				if (!types_compatible(field_type, value_type)) {
					char exp[128], got[128];
					type_record_name(field_type, exp, sizeof(exp));
					type_record_name(value_type, got, sizeof(got));
					compiler_panicf(compiler->parser, TYPE, "Cannot assign '%s' to field of type '%s'.", got, exp);
				}
			}

			if (field_index != -1) {
				emit_words(compiler, OP_SET_PROPERTY_INDEX, (uint16_t)field_index);
			} else {
				emit_words(compiler, OP_SET_PROPERTY, name_constant);
			}
			push_type_record(compiler, T_NIL);
			pop_type_record(compiler);

			pop(compiler->owner->current_module_record); // field_name
			pop(compiler->owner->current_module_record); // object_type
			return;
		}

		// Handle properties with compound op
		const int op = match_compound_op(compiler);
		if (op != -1) {
			if (object_type->base_type == STRUCT_TYPE && !field_type) {
				compiler_panicf(compiler->parser, NAME, "Struct has no field '%.*s'.", (int)method_name_token.length,
								method_name_token.start);
			}

			expression(compiler);
			ObjectTypeRecord *rhs_type = pop_type_record(compiler);
			check_compound_type_math(compiler, field_type ? field_type : T_ANY, rhs_type, op);

			if (field_index != -1) {
				emit_words(compiler, get_compound_opcode(compiler, OP_SET_PROPERTY_INDEX, op), (uint16_t)field_index);
			} else {
				emit_words(compiler, get_compound_opcode(compiler, OP_SET_PROPERTY, op), name_constant);
			}
			pop_type_record(compiler);
			push_type_record(compiler, rhs_type); // assignment leaves the value on the stack

			pop(compiler->owner->current_module_record); // field_name
			pop(compiler->owner->current_module_record); // object_type
			return;
		}
		pop(compiler->owner->current_module_record); // field_name
	}

	// OP_INVOKE
	if (match(compiler, CRUX_TOKEN_LEFT_PAREN)) {
		uint16_t arg_count = 0;
		ObjectTypeRecord *arg_types[UINT8_COUNT] = {0};

		// compiling arguments
		if (!check(compiler, CRUX_TOKEN_RIGHT_PAREN)) {
			do {
				if (arg_count >= UINT8_COUNT) {
					for (int i = 0; i < arg_count; i++)
						pop(compiler->owner->current_module_record);
					pop(compiler->owner->current_module_record); // object_type
					compiler_panic(compiler->parser, "Cannot have more than 255 arguments.", ARGUMENT_EXTENT);
					return;
				}

				expression(compiler);
				arg_types[arg_count] = pop_type_record(compiler);
				push(compiler->owner->current_module_record, OBJECT_VAL(arg_types[arg_count]));
				arg_count++;
			} while (match(compiler, CRUX_TOKEN_COMMA));
		}
		consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");

		ObjectTypeRecord **method_arg_types = NULL;
		ObjectTypeRecord *method_return = NULL;
		int method_arity = 0; // includes self as arg[0]
		bool method_found = false;

		if (object_type->base_type == STRUCT_TYPE) {
			const ObjectTypeTable *field_types = object_type->as.struct_type.field_types;
			const ObjectString *field_name = copy_string(compiler->owner, method_name_token.start,
														 method_name_token.length);
			ObjectTypeRecord *fn_type = NULL;
			if (type_table_get(field_types, field_name, &fn_type) && fn_type && fn_type->base_type == FUNCTION_TYPE) {
				method_arg_types = fn_type->as.function_type.arg_types;
				method_arity = fn_type->as.function_type.arg_count;
				method_return = fn_type->as.function_type.return_type;
				method_found = true;
			} else {
				compiler_panicf(compiler->parser, TYPE, "Struct field '%.*s' is not callable.",
								(int)method_name_token.length, method_name_token.start);
			}

		} else if (object_type->base_type != ANY_TYPE) {
			const CruxVM *vm = compiler->owner;
			const Table *type_table = NULL;

			switch (object_type->base_type) {
			case STRING_TYPE:
				type_table = &vm->string_type;
				break;
			case ARRAY_TYPE:
				type_table = &vm->array_type;
				break;
			case TABLE_TYPE:
				type_table = &vm->table_type;
				break;
			case ERROR_TYPE:
				type_table = &vm->error_type;
				break;
			case RESULT_TYPE:
				type_table = &vm->result_type;
				break;
			case OPTION_TYPE:
				type_table = &vm->option_type;
				break;
			case FILE_TYPE:
				type_table = &vm->file_type;
				break;
			case RANDOM_TYPE:
				type_table = &vm->random_type;
				break;
			case VECTOR_TYPE:
				type_table = &vm->vector_type;
				break;
			case COMPLEX_TYPE:
				type_table = &vm->complex_type;
				break;
			case MATRIX_TYPE:
				type_table = &vm->matrix_type;
				break;
			case RANGE_TYPE:
				type_table = &vm->range_type;
				break;
			case TUPLE_TYPE:
				type_table = &vm->tuple_type;
				break;
			case BUFFER_TYPE:
				type_table = &vm->buffer_type;
				break;
			default:
				break;
			}

			if (type_table) {
				stdlib_callable = lookup_stdlib_method(compiler, type_table, &method_name_token);
				if (stdlib_callable) {
					method_arg_types = stdlib_callable->arg_types;
					method_arity = stdlib_callable->arity;
					method_return = stdlib_callable->return_type;
					method_found = true;
				} else {
					char type_name[128];
					type_record_name(object_type, type_name, sizeof(type_name));
					compiler_panicf(compiler->parser, NAME, "'%s' has no method '%.*s'.", type_name,
									(int)method_name_token.length, method_name_token.start);
				}
			}
		}

		if (method_found && method_arg_types) {
			const int param_offset = (object_type->base_type == STRUCT_TYPE) ? 0 : 1;
			int user_params = method_arity - param_offset;
			if (user_params < 0)
				user_params = 0;

			if ((int)arg_count != user_params) {
				compiler_panicf(compiler->parser, ARGUMENT_MISMATCH, "Method '%.*s' expects %d argument(s), got %d.",
								(int)method_name_token.length, method_name_token.start, user_params, (int)arg_count);
			} else {
				for (int i = 0; i < (int)arg_count; i++) {
					ObjectTypeRecord *expected = method_arg_types[i + param_offset];
					ObjectTypeRecord *got_type = arg_types[i];
					if (expected && got_type && expected->base_type != ANY_TYPE && got_type->base_type != ANY_TYPE &&
						!types_compatible(expected, got_type)) {
						char exp_name[128], got_name[128];
						type_record_name(expected, exp_name, sizeof(exp_name));
						type_record_name(got_type, got_name, sizeof(got_name));
						compiler_panicf(compiler->parser, TYPE, "Argument %d type mismatch: expected '%s', got '%s'.",
										i + 1, exp_name, got_name);
					}
				}
			}
		}

		if (stdlib_callable) {
			const uint16_t callable_index = make_constant(compiler, OBJECT_VAL(stdlib_callable));
			emit_words(compiler, OP_INVOKE_STDLIB, callable_index);
			emit_word(compiler, arg_count);
		} else {
			emit_words(compiler, OP_INVOKE, name_constant);
			emit_word(compiler, arg_count);
		}

		pop_type_record(compiler);
		push_type_record(compiler, method_return ? method_return : T_ANY);

		for (int i = 0; i < (int)arg_count; i++) {
			pop(compiler->owner->current_module_record);
		}
		pop(compiler->owner->current_module_record); // object_type
		return;
	}

	// OP_GET_PROPERTY - only works on structs
	if (field_index != -1) {
		emit_words(compiler, OP_GET_PROPERTY_INDEX, (uint16_t)field_index);
	} else {
		emit_words(compiler, OP_GET_PROPERTY, name_constant);
	}

	ObjectTypeRecord *result_type = NULL;

	if (object_type->base_type == STRUCT_TYPE) {
		const ObjectTypeTable *field_types = object_type->as.struct_type.field_types;
		ObjectString *field_name = AS_CRUX_STRING(current_chunk(compiler)->constants.values[name_constant]);
		ObjectTypeRecord *field_type = NULL;
		if (type_table_get(field_types, field_name, &field_type)) {
			result_type = field_type;
		} else {
			compiler_panicf(compiler->parser, NAME, "Struct has no field '%.*s'.", (int)method_name_token.length,
							method_name_token.start);
		}
	}

	pop_type_record(compiler);
	push_type_record(compiler, result_type ? result_type : T_ANY);

	pop(compiler->owner->current_module_record); // object_type
}

void struct_instance(Compiler *compiler, const bool can_assign)
{
	consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected struct name to start initialization.");

	named_variable(compiler, compiler->parser->previous, can_assign);

	ObjectTypeRecord *struct_type = peek_type_record(compiler);

	const bool type_known = struct_type && struct_type->base_type != ANY_TYPE;
	if (type_known && struct_type->base_type != STRUCT_TYPE) {
		char got[128];
		type_record_name(struct_type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "'new' requires a struct type name, got '%s'.", got);
		pop_type_record(compiler);
		push_type_record(compiler, T_ANY);
		return;
	}

	if (!match(compiler, CRUX_TOKEN_LEFT_BRACE)) {
		compiler_panic(compiler->parser, "Expected '{' to start struct instance.", SYNTAX);
		pop_type_record(compiler);
		push_type_record(compiler, T_ANY);
		return;
	}

	int declared_field_count = type_known ? struct_type->as.struct_type.field_count : 0;
	bool *field_seen = NULL;
	if (declared_field_count > 0) {
		field_seen = ALLOCATE(compiler->owner, bool, declared_field_count);
		for (int i = 0; i < declared_field_count; i++) {
			field_seen[i] = false;
		}
	}

	uint16_t fieldCount = 0;
	emit_word(compiler, OP_STRUCT_INSTANCE_START);

	if (!match(compiler, CRUX_TOKEN_RIGHT_BRACE)) {
		do {
			if (fieldCount == UINT16_MAX) {
				compiler_panic(compiler->parser, "Too many fields in struct initializer.", SYNTAX);
				if (field_seen)
					FREE_ARRAY(compiler->owner, bool, field_seen, declared_field_count);
				pop_type_record(compiler);
				push_type_record(compiler, T_ANY);
				return;
			}

			consume(compiler, CRUX_TOKEN_IDENTIFIER,
					"Expected field name. Trailing commas after final field are not allowed.");
			ObjectString *fieldName = copy_string(compiler->owner, compiler->parser->previous.start,
												  compiler->parser->previous.length);
			push(compiler->owner->current_module_record, OBJECT_VAL(fieldName));

			consume(compiler, CRUX_TOKEN_EQUAL, "Expected '=' after struct field name.");

			expression(compiler);
			ObjectTypeRecord *value_type = pop_type_record(compiler);

			if (type_known) {
				const ObjectTypeTable *field_types = struct_type->as.struct_type.field_types;
				const ObjectStruct *definition = struct_type->as.struct_type.definition;

				// ensure field exists on the struct
				CruxValue field_index_val;
				if (!table_get(&definition->fields, fieldName, &field_index_val)) {
					compiler_panicf(compiler->parser, NAME, "Struct has no field '%.*s'.", (int)fieldName->byte_length,
									fieldName->chars);
				} else {
					// mark field as seen
					const int field_index = AS_INT(field_index_val);
					if (field_index >= 0 && field_index < declared_field_count) {
						if (field_seen[field_index]) {
							compiler_panicf(compiler->parser, NAME, "Field '%.*s' specified more than once.",
											(int)fieldName->byte_length, fieldName->chars);
						}
						field_seen[field_index] = true;
					}

					// Validate the value type against the declared field type.
					ObjectTypeRecord *declared_field_type = NULL;
					type_table_get(field_types, fieldName, &declared_field_type);

					if (declared_field_type && value_type && declared_field_type->base_type != ANY_TYPE &&
						value_type->base_type != ANY_TYPE) {
						if (!types_compatible(declared_field_type, value_type)) {
							char expected[128], got[128];
							type_record_name(declared_field_type, expected, sizeof(expected));
							type_record_name(value_type, got, sizeof(got));
							compiler_panicf(compiler->parser, TYPE, "Field '%.*s' expects type '%s', got '%s'.",
											(int)fieldName->byte_length, fieldName->chars, expected, got);
						}
					}
				}
			}

			const uint16_t fieldNameConstant = make_constant(compiler, OBJECT_VAL(fieldName));
			emit_words(compiler, OP_STRUCT_NAMED_FIELD, fieldNameConstant);

			pop(compiler->owner->current_module_record); // unroot fieldName
			fieldCount++;
		} while (match(compiler, CRUX_TOKEN_COMMA));
	}

	// Check for missing fields — every declared field must be provided.
	if (type_known && field_seen) {
		const ObjectStruct *definition = struct_type->as.struct_type.definition;

		for (int i = 0; i < declared_field_count; i++) {
			if (!field_seen[i]) {
				const char *missing = "<unknown>";
				int missing_len = 0;
				for (int e = 0; e < definition->fields.capacity; e++) {
					const Entry *entry = &definition->fields.entries[e];
					if (entry->key != NULL && AS_INT(entry->value) == i) {
						missing = entry->key->chars;
						missing_len = (int)entry->key->byte_length;
						break;
					}
				}
				compiler_panicf(compiler->parser, NAME, "Missing required field '%.*s' in struct initializer.",
								missing_len, missing);
				break; // Don't report all missing fields at once
			}
		}
		FREE_ARRAY(compiler->owner, bool, field_seen, declared_field_count);
	}

	if (fieldCount != 0) {
		consume(compiler, CRUX_TOKEN_RIGHT_BRACE, "Expected '}' after struct field list.");
	}

	emit_word(compiler, OP_STRUCT_INSTANCE_END);

	pop_type_record(compiler);
	push_type_record(compiler, struct_type ? struct_type : T_ANY);
}

void expression(Compiler *compiler)
{
	parse_precedence(compiler, PREC_ASSIGNMENT);
}

static void variable(Compiler *compiler, const bool can_assign)
{
	named_variable(compiler, compiler->parser->previous, can_assign);
}

void infix_call(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;

	const ObjectTypeRecord *func_type = peek_type_record(compiler);
	uint16_t arg_count = 0;
	ObjectTypeRecord *arg_types[UINT8_COUNT] = {0};

	if (!check(compiler, CRUX_TOKEN_RIGHT_PAREN)) {
		do {
			if (arg_count == UINT16_MAX) {
				// Prevent stack leak if we panic inside this loop
				for (int i = 0; i < arg_count; i++)
					pop(compiler->owner->current_module_record);
				compiler_panic(compiler->parser, "Cannot have more than 65535 arguments.", ARGUMENT_EXTENT);
				return;
			}
			expression(compiler);
			arg_types[arg_count] = pop_type_record(compiler);
			push(compiler->owner->current_module_record, OBJECT_VAL(arg_types[arg_count]));
			arg_count++;
		} while (match(compiler, CRUX_TOKEN_COMMA));
	}
	consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after argument list.");

	emit_words(compiler, OP_CALL, arg_count);

	// Type-check when the callee is statically known.
	if (func_type && func_type->base_type == FUNCTION_TYPE) {
		const int expected_count = func_type->as.function_type.arg_count;

		if ((int)arg_count != expected_count) {
			compiler_panicf(compiler->parser, ARGUMENT_MISMATCH, "Expected %d argument(s), got %d.", expected_count,
							(int)arg_count);
		} else {
			for (int i = 0; i < (int)arg_count; i++) {
				ObjectTypeRecord *expected = func_type->as.function_type.arg_types[i];
				ObjectTypeRecord *got = arg_types[i];
				if (expected && got && expected->base_type != ANY_TYPE && got->base_type != ANY_TYPE) {
					if (!types_compatible(expected, got)) {
						char exp_name[128], got_name[128];
						type_record_name(expected, exp_name, sizeof(exp_name));
						type_record_name(got, got_name, sizeof(got_name));
						compiler_panicf(compiler->parser, TYPE, "Argument %d type mismatch: expected '%s', got '%s'.",
										i + 1, exp_name, got_name);
					}
				}
			}
		}

		ObjectTypeRecord *ret = func_type->as.function_type.return_type;
		pop_type_record(compiler);
		push_type_record(compiler, ret ? ret : T_ANY);
	} else {
		// unknown callee type
		pop_type_record(compiler);
		push_type_record(compiler, T_ANY);
	}
	for (int i = 0; i < arg_count; i++) {
		pop(compiler->owner->current_module_record);
	}
}

static void literal(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	switch (compiler->parser->previous.type) {
	case CRUX_TOKEN_FALSE:
		emit_word(compiler, OP_FALSE);
		push_type_record(compiler, T_BOOL);
		break;
	case CRUX_TOKEN_NIL:
		compiler->current_narrowing.tracked_literal_type = T_NIL;
		emit_word(compiler, OP_NIL);
		push_type_record(compiler, T_NIL);
		break;
	case CRUX_TOKEN_TRUE:
		emit_word(compiler, OP_TRUE);
		push_type_record(compiler, T_BOOL);
		break;
	default:
		return; // unreachable
	}
}

static void grouping(Compiler *compiler, bool can_assign)
{
	(void)can_assign;
	expression(compiler);
	consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

ParseRule rules[] = {
	[CRUX_TOKEN_LEFT_PAREN] = {grouping, infix_call, NULL, PREC_CALL},
	[CRUX_TOKEN_RIGHT_PAREN] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_LEFT_BRACE] = {table_literal, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_RIGHT_BRACE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_LEFT_SQUARE] = {array_literal, collection_index, NULL, PREC_CALL},
	[CRUX_TOKEN_RIGHT_SQUARE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_DOLLAR_LEFT_SQUARE] = {tuple_literal, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_COMMA] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_DOT] = {NULL, dot, NULL, PREC_CALL},
	[CRUX_TOKEN_COLON_COLON] = {NULL, colon_colon, NULL, PREC_CALL},
	[CRUX_TOKEN_MINUS] = {unary, binary, NULL, PREC_TERM},
	[CRUX_TOKEN_PLUS] = {NULL, binary, NULL, PREC_TERM},
	[CRUX_TOKEN_SEMICOLON] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_SLASH] = {NULL, binary, NULL, PREC_FACTOR},
	[CRUX_TOKEN_BACKSLASH] = {NULL, binary, NULL, PREC_FACTOR},
	[CRUX_TOKEN_STAR] = {NULL, binary, NULL, PREC_FACTOR},
	[CRUX_TOKEN_STAR_STAR] = {NULL, binary, NULL, PREC_FACTOR},
	[CRUX_TOKEN_PERCENT] = {NULL, binary, NULL, PREC_FACTOR},
	[CRUX_TOKEN_LEFT_SHIFT] = {NULL, binary, NULL, PREC_SHIFT},
	[CRUX_TOKEN_RIGHT_SHIFT] = {NULL, binary, NULL, PREC_SHIFT},
	[CRUX_TOKEN_AMPERSAND] = {NULL, binary, NULL, PREC_BITWISE_AND},
	[CRUX_TOKEN_CARET] = {NULL, binary, NULL, PREC_BITWISE_XOR},
	[CRUX_TOKEN_PIPE] = {NULL, binary, NULL, PREC_BITWISE_OR},
	[CRUX_TOKEN_NOT] = {unary, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_BANG_EQUAL] = {NULL, binary, NULL, PREC_EQUALITY},
	[CRUX_TOKEN_EQUAL] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_EQUAL_EQUAL] = {NULL, binary, NULL, PREC_EQUALITY},
	[CRUX_TOKEN_GREATER] = {NULL, binary, NULL, PREC_COMPARISON},
	[CRUX_TOKEN_GREATER_EQUAL] = {NULL, binary, NULL, PREC_COMPARISON},
	[CRUX_TOKEN_LESS] = {NULL, binary, NULL, PREC_COMPARISON},
	[CRUX_TOKEN_LESS_EQUAL] = {NULL, binary, NULL, PREC_COMPARISON},
	[CRUX_TOKEN_IDENTIFIER] = {variable, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_STRING] = {string, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_INT] = {number, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_FLOAT] = {number, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_BINARY_INT] = {binary_number, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_HEX_INT] = {hex_number, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_CONTINUE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_BREAK] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_AND] = {NULL, and_, NULL, PREC_AND},
	[CRUX_TOKEN_ELSE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_FALSE] = {literal, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_FOR] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_FN] = {anonymous_function, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_IF] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_NIL] = {literal, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_OR] = {NULL, or_, NULL, PREC_OR},
	[CRUX_TOKEN_RETURN] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_TRUE] = {literal, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_LET] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_USE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_FROM] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_PUB] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_WHILE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_ERROR] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_DEFAULT] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_EQUAL_ARROW] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_OK] = {ok_expression, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_ERR] = {err_expression, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_SOME] = {some_expression, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_NONE] = {none_expression, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_MATCH] = {match_expression, NULL, NULL, PREC_PRIMARY},
	[CRUX_TOKEN_TYPEOF] = {typeof_expression, NULL, NULL, PREC_UNARY},
	[CRUX_TOKEN_STRUCT] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_NEW] = {struct_instance, NULL, NULL, PREC_UNARY},
	[CRUX_TOKEN_EOF] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_QUESTION_MARK] = {NULL, NULL, result_unwrap, PREC_CALL},
	[CRUX_TOKEN_AS] = {NULL, type_coerce, NULL, PREC_COERCE},
	[CRUX_TOKEN_TILDE] = {unary, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_IN] = {NULL, binary, NULL, PREC_IN},
	[CRUX_TOKEN_PANIC] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_NIL_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_BOOL_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_INT_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_FLOAT_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_STRING_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_ARRAY_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_TABLE_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_ERROR_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_RESULT_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_RANDOM_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_FILE_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_STRUCT_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_VECTOR_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_COMPLEX_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_MATRIX_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_TUPLE_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_BUFFER_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_RANGE_TYPE] = {NULL, NULL, NULL, PREC_NONE},
	[CRUX_TOKEN_ANY_TYPE] = {NULL, NULL, NULL, PREC_NONE},
};

ParseRule *get_rule(const CruxTokenType type)
{
	return &rules[type];
}

void parse_precedence(Compiler *compiler, const Precedence precedence)
{
	advance(compiler);
	ParseFn prefixRule;
	if (is_identifier_like(compiler->parser->previous.type)) {
		prefixRule = get_rule(CRUX_TOKEN_IDENTIFIER)->prefix;
	} else {
		prefixRule = get_rule(compiler->parser->previous.type)->prefix;
	}
	if (prefixRule == NULL) {
		compiler_panic(compiler->parser, "Expected expression.", SYNTAX);
		return;
	}

	const bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefixRule(compiler, can_assign);

	while (precedence <= get_rule(compiler->parser->current.type)->precedence) {
		advance(compiler);
		const ParseRule *rule = get_rule(compiler->parser->previous.type);
		if (rule->infix != NULL) {
			rule->infix(compiler, can_assign);
		} else if (rule->postfix != NULL) {
			rule->postfix(compiler, can_assign);
		}
	}

	if (can_assign && match(compiler, CRUX_TOKEN_EQUAL)) {
		compiler_panic(compiler->parser, "Invalid Assignment Target", SYNTAX);
	}
}
