#include "compiler/compiler_core.h"

/**Pre-scanner
 * A two-sub-pass scanner that collects top-level struct and
 * function signatures before the main compilation pass begins.
 * This gives the compiler knowledge of forward-referenced names
 *
 * First pass collects struct declarations
 * Second pass collects function signatures
 */

// Advance the pre-scanner, skipping error tokens silently.
static void pre_advance(const Compiler *compiler)
{
	compiler->parser->previous = compiler->parser->current;
	for (;;) {
		compiler->parser->current = scan_token(compiler->parser->scanner);
		if (compiler->parser->current.type != CRUX_TOKEN_ERROR)
			break;
	}
}

/**
 * Skip a balanced brace block { ... } starting at the current token (which
 * must be CRUX_TOKEN_LEFT_BRACE). Handles nesting.
 */
static void pre_skip_block(const Compiler *compiler)
{
	if (compiler->parser->current.type != CRUX_TOKEN_LEFT_BRACE)
		return;
	pre_advance(compiler); // consume '{'
	int depth = 1;
	while (depth > 0 && compiler->parser->current.type != CRUX_TOKEN_EOF) {
		if (compiler->parser->current.type == CRUX_TOKEN_LEFT_BRACE)
			depth++;
		if (compiler->parser->current.type == CRUX_TOKEN_RIGHT_BRACE)
			depth--;
		pre_advance(compiler);
	}
}

// Skip a balanced parenthesis group ( ... ) starting at current token.
static void pre_skip_parens(const Compiler *compiler)
{
	if (compiler->parser->current.type != CRUX_TOKEN_LEFT_PAREN)
		return;
	pre_advance(compiler); // consume '('
	int depth = 1;
	while (depth > 0 && compiler->parser->current.type != CRUX_TOKEN_EOF) {
		if (compiler->parser->current.type == CRUX_TOKEN_LEFT_PAREN)
			depth++;
		if (compiler->parser->current.type == CRUX_TOKEN_RIGHT_PAREN)
			depth--;
		pre_advance(compiler);
	}
}

/**
 * Advance past a type annotation starting at the current token.
 * Handles nested brackets and parentheses (e.g. Array[Int], (Int)->Bool,
 * Table[String:Int], union types separated by '|').
 */
static void pre_skip_type(Compiler *compiler)
{
	for (;;) {
		const CruxTokenType t = compiler->parser->current.type;

		if (t == CRUX_TOKEN_LEFT_PAREN) {
			// Function type: (T, T) -> T
			pre_skip_parens(compiler);
			// consume '->'
			if (compiler->parser->current.type == CRUX_TOKEN_ARROW)
				pre_advance(compiler);
			pre_skip_type(compiler); // return type
		} else if (t == CRUX_TOKEN_SHAPE) {
			pre_advance(compiler); // consume 'shape'
			pre_skip_block(compiler); // skip '{ ... }'
		} else if (t == CRUX_TOKEN_INT_TYPE || t == CRUX_TOKEN_FLOAT_TYPE || t == CRUX_TOKEN_BOOL_TYPE ||
				   t == CRUX_TOKEN_STRING_TYPE || t == CRUX_TOKEN_NIL_TYPE || t == CRUX_TOKEN_ANY_TYPE ||
				   t == CRUX_TOKEN_ARRAY_TYPE || t == CRUX_TOKEN_TABLE_TYPE || t == CRUX_TOKEN_VECTOR_TYPE ||
				   t == CRUX_TOKEN_MATRIX_TYPE || t == CRUX_TOKEN_BUFFER_TYPE || t == CRUX_TOKEN_ERROR_TYPE ||
				   t == CRUX_TOKEN_RESULT_TYPE || t == CRUX_TOKEN_RANGE_TYPE || t == CRUX_TOKEN_TUPLE_TYPE ||
				   t == CRUX_TOKEN_COMPLEX_TYPE || t == CRUX_TOKEN_SET_TYPE || t == CRUX_TOKEN_RANDOM_TYPE ||
				   t == CRUX_TOKEN_FILE_TYPE || t == CRUX_TOKEN_IDENTIFIER || t == CRUX_TOKEN_NEVER_TYPE) {
			pre_advance(compiler); // consume the base type token
			// Optional subscript: Array[Int], Table[K,V], etc.
			if (compiler->parser->current.type == CRUX_TOKEN_LEFT_SQUARE) {
				pre_advance(compiler); // consume '['
				int depth = 1;
				while (depth > 0 && compiler->parser->current.type != CRUX_TOKEN_EOF) {
					if (compiler->parser->current.type == CRUX_TOKEN_LEFT_SQUARE)
						depth++;
					if (compiler->parser->current.type == CRUX_TOKEN_RIGHT_SQUARE)
						depth--;
					pre_advance(compiler);
				}
			}
		} else {
			// Unknown/unsupported — stop.
			break;
		}

		// Union continuation: T | T | ...
		if (compiler->parser->current.type == CRUX_TOKEN_PIPE) {
			pre_advance(compiler); // consume '|'
			continue; // parse next variant
		}
		break;
	}
}

// Collect a single top-level type alias declaration.
// On entry, parser.current is CRUX_TOKEN_TYPE (already consumed by caller).
static void pre_collect_type(Compiler *compiler)
{
	if (compiler->parser->current.type != CRUX_TOKEN_IDENTIFIER)
		return;
	Token name_token = compiler->parser->current;
	pre_advance(compiler);

	if (compiler->parser->current.type != CRUX_TOKEN_EQUAL)
		return;
	pre_advance(compiler); // consume '='

	// parse_type_record uses the global current compiler and parser.
	ObjectTypeRecord *resolved_type = parse_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(resolved_type));

	// consume ';'
	if (compiler->parser->current.type == CRUX_TOKEN_SEMICOLON)
		pre_advance(compiler);

	ObjectString *type_name = copy_string(compiler->owner, name_token.start, name_token.length);
	push(compiler->owner->current_module_record, OBJECT_VAL(type_name));
	type_table_set(compiler->type_table, type_name, resolved_type);
	pop(compiler->owner->current_module_record); // type_name
	pop(compiler->owner->current_module_record); // resolved_type
}

// Collect a single top-level struct declaration into pre_compiler's type_table.
// On entry parser.current is CRUX_TOKEN_STRUCT (already consumed by caller).
static void pre_collect_struct(Compiler *compiler)
{
	// Consume struct name.
	if (compiler->parser->current.type != CRUX_TOKEN_IDENTIFIER)
		return;
	const Token name_token = compiler->parser->current;
	pre_advance(compiler);

	// Expect '{' to start the struct body.
	if (compiler->parser->current.type != CRUX_TOKEN_LEFT_BRACE)
		return;
	pre_advance(compiler); // consume '{'

	ObjectTypeTable *field_types = new_type_table(compiler->owner, INITIAL_TYPE_TABLE_SIZE);
	push(compiler->owner->current_module_record, OBJECT_VAL(field_types));
	int field_count = 0;

	ObjectString *struct_name = copy_string(compiler->owner, name_token.start, name_token.length);
	push(compiler->owner->current_module_record, OBJECT_VAL(struct_name));

	ObjectStruct *struct_obj = new_struct_type(compiler->owner, struct_name);
	push(compiler->owner->current_module_record, OBJECT_VAL(struct_obj));

	// Register the struct type before parsing fields so self-referential fields can resolve during the pre-pass.
	ObjectTypeRecord *struct_type = new_struct_type_rec(compiler->owner, struct_obj, field_types, 0);
	push(compiler->owner->current_module_record, OBJECT_VAL(struct_type));
	type_table_set(compiler->type_table, struct_name, struct_type);

	while (compiler->parser->current.type != CRUX_TOKEN_RIGHT_BRACE &&
		   compiler->parser->current.type != CRUX_TOKEN_EOF) {
		// Field name
		if (compiler->parser->current.type != CRUX_TOKEN_IDENTIFIER)
			break;
		const Token field_tok = compiler->parser->current;
		ObjectString *field_name = copy_string(compiler->owner, field_tok.start, field_tok.length);
		push(compiler->owner->current_module_record, OBJECT_VAL(field_name));
		pre_advance(compiler);

		ObjectTypeRecord *field_type = NULL;
		if (compiler->parser->current.type == CRUX_TOKEN_COLON) {
			pre_advance(compiler); // consume ':'
			field_type = parse_type_record(compiler);
		} else {
			field_type = T_ANY;
		}
		push(compiler->owner->current_module_record, OBJECT_VAL(field_type));
		type_table_set(field_types, field_name, field_type);
		table_set(compiler->owner, &struct_obj->fields, field_name, INT_VAL(field_count));
		field_count++;

		// Allow trailing comma between fields. Min compiler will catch this later.
		if (compiler->parser->current.type == CRUX_TOKEN_COMMA)
			pre_advance(compiler);
	}

	// Consume '}'.
	if (compiler->parser->current.type == CRUX_TOKEN_RIGHT_BRACE)
		pre_advance(compiler);

	struct_type->as.struct_type.field_count = field_count;

	pop(compiler->owner->current_module_record); // struct type
	for (int i = 0; i < field_count; i++) {
		pop(compiler->owner->current_module_record); // field type
		pop(compiler->owner->current_module_record); // field name
	}
	pop(compiler->owner->current_module_record); // struct name
	pop(compiler->owner->current_module_record); // struct_obj
	pop(compiler->owner->current_module_record); // field_types
}

// Collect a single top-level function signature into pre_compiler's type_table.
static void pre_collect_function(Compiler *compiler)
{
	if (compiler->parser->current.type != CRUX_TOKEN_IDENTIFIER)
		return;
	const Token fn_name_token = compiler->parser->current;
	pre_advance(compiler);

	if (compiler->parser->current.type != CRUX_TOKEN_LEFT_PAREN)
		return;
	pre_advance(compiler); // consume '('

	int param_cap = 4;
	int param_count = 0;
	ObjectTypeRecord **param_types = ALLOCATE(compiler->owner, ObjectTypeRecord *, param_cap);
	if (!param_types) {
		return;
	}

	while (compiler->parser->current.type != CRUX_TOKEN_RIGHT_PAREN &&
		   compiler->parser->current.type != CRUX_TOKEN_EOF) {
		// Parameter name (identifier).
		if (compiler->parser->current.type != CRUX_TOKEN_IDENTIFIER) {
			FREE_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_count);
			return;
		}
		pre_advance(compiler); // consume param name

		ObjectTypeRecord *param_type = NULL;
		if (compiler->parser->current.type == CRUX_TOKEN_COLON) {
			pre_advance(compiler); // consume ':'
			param_type = parse_type_record(compiler);
		} else {
			param_type = T_ANY;
		}
		push(compiler->owner->current_module_record, OBJECT_VAL(param_type));

		if (param_count == param_cap) {
			const int old_cap = param_cap;
			param_cap = GROW_CAPACITY(param_cap);
			ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, old_cap, param_cap);
			if (!grown) {
				pop(compiler->owner->current_module_record);
				FREE_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, old_cap);
				return;
			}
			param_types = grown;
		}
		param_types[param_count++] = param_type;

		if (compiler->parser->current.type == CRUX_TOKEN_COMMA)
			pre_advance(compiler);
	}

	// shrink to actual size
	param_types = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_cap, param_count);

	if (compiler->parser->current.type == CRUX_TOKEN_RIGHT_PAREN)
		pre_advance(compiler);

	ObjectTypeRecord *return_type = NULL;
	if (compiler->parser->current.type == CRUX_TOKEN_ARROW) {
		pre_advance(compiler); // consume '->'
		return_type = parse_type_record(compiler);
	} else {
		return_type = T_ANY;
	}
	push(compiler->owner->current_module_record, OBJECT_VAL(return_type));

	ObjectTypeRecord *fn_type = new_function_type_rec(compiler->owner, param_types, param_count, return_type);
	push(compiler->owner->current_module_record, OBJECT_VAL(fn_type));
	ObjectString *fn_name = copy_string(compiler->owner, fn_name_token.start, fn_name_token.length);
	push(compiler->owner->current_module_record, OBJECT_VAL(fn_name));
	type_table_set(compiler->type_table, fn_name, fn_type);

	// Skip the function body
	pre_skip_block(compiler);
	pop(compiler->owner->current_module_record); // fn_name
	pop(compiler->owner->current_module_record); // fn_type
	pop(compiler->owner->current_module_record); // return type
	for (int i = 0; i < param_count; i++) {
		pop(compiler->owner->current_module_record); // param_type
	}
}

/**
 * Run a forward-scan sub-pass.
 * collects structs or functions based on the collect_structs flag.
 */
static void pre_scan_pass(Compiler *compiler, const bool collect_structs)
{
	// Reset parser to a clean state
	compiler->parser->had_error = false;
	compiler->parser->panic_mode = false;

	pre_advance(compiler);

	while (compiler->parser->current.type != CRUX_TOKEN_EOF) {
		const CruxTokenType t = compiler->parser->current.type;

		if (t == CRUX_TOKEN_STRUCT) {
			pre_advance(compiler); // consume 'struct'
			if (collect_structs) {
				pre_collect_struct(compiler);
			} else {
				// Skip: name + block
				if (compiler->parser->current.type == CRUX_TOKEN_IDENTIFIER)
					pre_advance(compiler);
				pre_skip_block(compiler);
			}

		} else if (t == CRUX_TOKEN_TYPE) {
			pre_advance(compiler); // consume 'type'
			if (collect_structs) {
				pre_collect_type(compiler);
			} else {
				// Skip: name + '=' + type + ';'
				if (compiler->parser->current.type == CRUX_TOKEN_IDENTIFIER)
					pre_advance(compiler);
				if (compiler->parser->current.type == CRUX_TOKEN_EQUAL)
					pre_advance(compiler);
				pre_skip_type(compiler);
				if (compiler->parser->current.type == CRUX_TOKEN_SEMICOLON)
					pre_advance(compiler);
			}

		} else if (t == CRUX_TOKEN_FN) {
			pre_advance(compiler); // consume 'fn'
			if (!collect_structs) {
				pre_collect_function(compiler);
			} else {
				// Skip: name + parens + optional ->T + block
				if (compiler->parser->current.type == CRUX_TOKEN_IDENTIFIER)
					pre_advance(compiler);
				pre_skip_parens(compiler);
				if (compiler->parser->current.type == CRUX_TOKEN_ARROW) {
					pre_advance(compiler);
					pre_skip_type(compiler);
				}
				pre_skip_block(compiler);
			}

		} else if (t == CRUX_TOKEN_PUB) {
			pre_advance(compiler); // consume 'pub'
			// pub fn ... or pub struct ...
			if (compiler->parser->current.type == CRUX_TOKEN_FN) {
				pre_advance(compiler); // consume 'fn'
				if (!collect_structs) {
					pre_collect_function(compiler);
				} else {
					if (compiler->parser->current.type == CRUX_TOKEN_IDENTIFIER)
						pre_advance(compiler);
					pre_skip_parens(compiler);
					if (compiler->parser->current.type == CRUX_TOKEN_ARROW) {
						pre_advance(compiler);
						pre_skip_type(compiler);
					}
					pre_skip_block(compiler);
				}
			} else if (compiler->parser->current.type == CRUX_TOKEN_STRUCT) {
				pre_advance(compiler); // consume 'struct'
				if (collect_structs) {
					pre_collect_struct(compiler);
				} else {
					if (compiler->parser->current.type == CRUX_TOKEN_IDENTIFIER)
						pre_advance(compiler);
					pre_skip_block(compiler);
				}
			} else if (compiler->parser->current.type == CRUX_TOKEN_TYPE) {
				pre_advance(compiler); // consume 'type'
				if (collect_structs) {
					pre_collect_type(compiler);
				} else {
					if (compiler->parser->current.type == CRUX_TOKEN_IDENTIFIER)
						pre_advance(compiler);
					if (compiler->parser->current.type == CRUX_TOKEN_EQUAL)
						pre_advance(compiler);
					pre_skip_type(compiler);
					if (compiler->parser->current.type == CRUX_TOKEN_SEMICOLON)
						pre_advance(compiler);
				}
			} else {
				pre_advance(compiler); // skip unknown pub token
			}

		} else {
			// Not a top-level fn/struct — skip the token.
			pre_advance(compiler);
		}
	}
}

// Run both pre-scan sub-passes and merge results into `dest`.
// The scanner must be initialized before calling.
void pre_scan(Compiler *compiler, char *source, ObjectTypeTable *dest)
{
	// Sub-pass 1: collect struct declarations
	Compiler pre_compiler_structs = {0};
	init_compiler(compiler->owner, &pre_compiler_structs, compiler, TYPE_SCRIPT);
	init_scanner(pre_compiler_structs.parser->scanner, source);

	pre_compiler_structs.parser->had_error = false;
	pre_compiler_structs.parser->panic_mode = false;
	pre_compiler_structs.parser->source = source;

	pre_scan_pass(&pre_compiler_structs, true);

	push(compiler->owner->current_module_record, OBJECT_VAL(pre_compiler_structs.type_table));
	compiler->enclosed = NULL;
	free(pre_compiler_structs.parser->scanner);
	free(pre_compiler_structs.parser);

	// Sub-pass 2: collect function signatures
	Compiler pre_compiler_fns = {0};
	init_compiler(compiler->owner, &pre_compiler_fns, compiler, TYPE_SCRIPT);
	init_scanner(pre_compiler_fns.parser->scanner, source);
	pre_compiler_fns.parser->had_error = false;
	pre_compiler_fns.parser->panic_mode = false;

	type_table_add_all(pre_compiler_structs.type_table, pre_compiler_fns.type_table);

	pre_scan_pass(&pre_compiler_fns, false);

	push(compiler->owner->current_module_record, OBJECT_VAL(pre_compiler_fns.type_table));
	compiler->enclosed = NULL;
	free(pre_compiler_fns.parser->scanner);
	free(pre_compiler_fns.parser);

	type_table_add_all(pre_compiler_structs.type_table, dest);
	type_table_add_all(pre_compiler_fns.type_table, dest);

	pop(compiler->owner->current_module_record); // pre_compiler_fns.type_table
	pop(compiler->owner->current_module_record); // pre_compiler_structs.type_table
}
