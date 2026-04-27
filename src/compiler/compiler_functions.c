#include "compiler/compiler_core.h"
#include "compiler/compiler_helpers.h"
#include "compiler/compiler_statements.h"
#include "panic.h"

void function(Compiler *compiler, const FunctionType type, ObjectTypeRecord *self_type,
					 ObjectString *recursive_name, int recursive_global_index)
{
	Compiler function_compiler = {0};
	if (!init_compiler(compiler->owner, &function_compiler, compiler, type)) {
		compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
		return;
	}
	if (type == TYPE_METHOD && self_type) {
		function_compiler.locals[0].type = self_type;
	}
	if (recursive_name != NULL && recursive_global_index != -1) {
		table_set(compiler->owner, &function_compiler.globals, recursive_name, INT_VAL(recursive_global_index));
	}
	begin_scope(&function_compiler);

	int param_capacity = 4;
	int param_count = 0;
	ObjectTypeRecord **param_types = ALLOCATE(compiler->owner, ObjectTypeRecord *, param_capacity);
	if (!param_types) {
		compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
		return;
	}

	consume(compiler, CRUX_TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if (!check(&function_compiler, CRUX_TOKEN_RIGHT_PAREN)) {
		do {
			function_compiler.function->arity++;
			if (function_compiler.function->arity > UINT8_MAX) {
				compiler_panic(compiler->parser,
							   "Functions cannot have more than "
							   "255 arguments.",
							   ARGUMENT_EXTENT);
			}
			const uint16_t constant = parse_variable(&function_compiler, "Expected parameter name.");

			ObjectTypeRecord *param_type = NULL;
			if (match(&function_compiler, CRUX_TOKEN_COLON)) {
				param_type = parse_type_record(&function_compiler);
			} else {
				param_type = T_ANY;
			}

			function_compiler.locals[function_compiler.local_count - 1].type = param_type;

			if (param_count == param_capacity) {
				param_capacity = GROW_CAPACITY(param_capacity);
				ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_count,
													  param_capacity);
				if (!grown) {
					FREE_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_count);
					compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
					return;
				}
				param_types = grown;
			}
			param_types[param_count++] = param_type;

			define_variable(&function_compiler, constant, false);
		} while (match(compiler, CRUX_TOKEN_COMMA));
	}

	param_types = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_capacity, param_count);

	consume(&function_compiler, CRUX_TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

	ObjectTypeRecord *annotated_return_type = NULL;
	if (match(&function_compiler, CRUX_TOKEN_ARROW)) {
		annotated_return_type = parse_type_record(&function_compiler);
	} else {
		annotated_return_type = T_ANY;
	}
	function_compiler.return_type = annotated_return_type;

	consume(&function_compiler, CRUX_TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block(&function_compiler);

	if (!function_compiler.has_return && annotated_return_type && annotated_return_type->base_type != NIL_TYPE &&
		annotated_return_type->base_type != ANY_TYPE) {
		char expected[128];
		type_record_name(annotated_return_type, expected, sizeof(expected));
		compiler_panicf(compiler->parser, TYPE, "Function expects to return '%s' but has no return statement.",
						expected);
	}

	ObjectFunction *fn = end_compiler(&function_compiler);

	push(compiler->owner->current_module_record, OBJECT_VAL(fn));
	push(compiler->owner->current_module_record, OBJECT_VAL(annotated_return_type));
	for (int i = 0; i < param_count; i++) {
		push(compiler->owner->current_module_record, OBJECT_VAL(param_types[i]));
	}

	emit_words(compiler, OP_CLOSURE, make_constant(compiler, OBJECT_VAL(fn)));

	for (int i = 0; i < fn->upvalue_count; i++) {
		emit_word(compiler, function_compiler.upvalues[i].is_local ? 1 : 0);
		emit_word(compiler, function_compiler.upvalues[i].index);
	}

	ObjectTypeRecord *func_type = new_function_type_rec(compiler->owner, param_types, param_count,
														annotated_return_type);
	push_type_record(compiler, func_type);

	for (int i = 0; i < param_count; i++) {
		pop(compiler->owner->current_module_record); // param_types[i]
	}
	pop(compiler->owner->current_module_record); // annotated_return_type
	pop(compiler->owner->current_module_record); // fn
}

void fn_declaration(Compiler *compiler, const bool is_public)
{
	const uint16_t global = parse_variable(compiler, "Expected function name.");

	const Token fn_name_token = compiler->parser->previous;
	ObjectString *name_str = copy_string(compiler->owner, fn_name_token.start, fn_name_token.length);

	push(compiler->owner->current_module_record, OBJECT_VAL(name_str));

	const int local_index = (compiler->scope_depth > 0) ? compiler->local_count - 1 : -1;
	int reserved_global_index = -1;

	if (compiler->scope_depth == 0) {
		reserved_global_index = compiler->global_count;
		if (!table_set(compiler->owner, &compiler->globals, name_str, INT_VAL(reserved_global_index))) {
			compiler_panicf(compiler->parser, NAME, "Cannot redefine global function '%s'.", name_str->chars);
		} else {
			compiler->global_count++;
		}
	}

	mark_initialized(compiler);
	function(compiler, TYPE_FUNCTION, NULL, name_str, reserved_global_index);

	ObjectTypeRecord *fn_type = pop_type_record(compiler);

	push(compiler->owner->current_module_record, OBJECT_VAL(fn_type));

	if (is_public || (compiler->owner->current_module_record && compiler->owner->current_module_record->is_repl)) {
		type_table_set(compiler->owner->current_module_record->types, name_str, fn_type);
	}

	if (fn_type) {
		if (compiler->scope_depth == 0) {
			type_table_set(compiler->type_table, name_str, fn_type);
		} else {
			compiler->locals[local_index].type = fn_type;
		}
	}

	if (reserved_global_index != -1) {
		if (is_public) {
			emit_words(compiler, OP_DEFINE_PUB_GLOBAL, reserved_global_index);
			emit_word(compiler, global);
		} else {
			emit_words(compiler, OP_DEFINE_GLOBAL, reserved_global_index);
		}
	} else {
		define_variable(compiler, global, is_public);
	}

	pop(compiler->owner->current_module_record); // fn_type
	pop(compiler->owner->current_module_record); // name_str
}

void anonymous_function(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	Compiler function_compiler = {0};
	if (!init_compiler(compiler->owner, &function_compiler, compiler, TYPE_ANONYMOUS)) {
		compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
		return;
	}

	int param_capacity = 4;
	int param_count = 0;
	ObjectTypeRecord **param_types = ALLOCATE(compiler->owner, ObjectTypeRecord *, param_capacity);
	if (!param_types) {
		compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
		return;
	}

	begin_scope(&function_compiler);
	consume(&function_compiler, CRUX_TOKEN_LEFT_PAREN, "Expected '(' to start argument list.");

	if (!check(&function_compiler, CRUX_TOKEN_RIGHT_PAREN)) {
		do {
			function_compiler.function->arity++;
			if (function_compiler.function->arity > UINT8_MAX) {
				compiler_panic(compiler->parser,
							   "Functions cannot have more than "
							   "255 arguments.",
							   ARGUMENT_EXTENT);
			}
			const uint16_t constant = parse_variable(&function_compiler, "Expected parameter name.");

			ObjectTypeRecord *param_type = NULL;
			if (match(&function_compiler, CRUX_TOKEN_COLON)) {
				param_type = parse_type_record(&function_compiler);
			} else {
				param_type = T_ANY;
			}

			// Store on the local slot that parse_variable created
			function_compiler.locals[function_compiler.local_count - 1].type = param_type;

			if (param_count == param_capacity) {
				const int old_cap = param_capacity;
				param_capacity = GROW_CAPACITY(param_capacity);
				ObjectTypeRecord **grown = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, old_cap,
													  param_capacity);
				if (!grown) {
					FREE_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, old_cap);
					compiler_panic(compiler->parser, "Memory allocation failed.", MEMORY);
					return;
				}
				param_types = grown;
			}
			param_types[param_count++] = param_type;

			define_variable(&function_compiler, constant, false);
		} while (match(compiler, CRUX_TOKEN_COMMA));
	}

	consume(&function_compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after argument list.");

	ObjectTypeRecord *annotated_return_type = NULL;
	if (match(&function_compiler, CRUX_TOKEN_ARROW)) {
		annotated_return_type = parse_type_record(&function_compiler);
	} else {
		annotated_return_type = T_ANY;
	}

	function_compiler.return_type = annotated_return_type;

	consume(&function_compiler, CRUX_TOKEN_LEFT_BRACE, "Expected '{' before function body.");
	block(&function_compiler);

	if (!function_compiler.has_return && annotated_return_type && annotated_return_type->base_type != NIL_TYPE &&
		annotated_return_type->base_type != ANY_TYPE) {
		char expected[128];
		type_record_name(annotated_return_type, expected, sizeof(expected));
		compiler_panicf(function_compiler.parser, TYPE, "Function expects to return '%s' but has no return statement.",
						expected);
	}

	ObjectFunction *fn = end_compiler(&function_compiler);

	push(compiler->owner->current_module_record, OBJECT_VAL(fn));
	push(compiler->owner->current_module_record, OBJECT_VAL(annotated_return_type));
	for (int i = 0; i < param_count; i++) {
		push(compiler->owner->current_module_record, OBJECT_VAL(param_types[i]));
	}

	const uint16_t constantIndex = make_constant(compiler, OBJECT_VAL(fn));
	emit_words(compiler, OP_ANON_FUNCTION, constantIndex);

	for (int i = 0; i < fn->upvalue_count; i++) {
		emit_word(compiler, function_compiler.upvalues[i].is_local ? 1 : 0);
		emit_word(compiler, function_compiler.upvalues[i].index);
	}

	param_types = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, param_types, param_capacity, param_count);

	ObjectTypeRecord *func_type = new_function_type_rec(compiler->owner, param_types, param_count,
														annotated_return_type);
	push_type_record(compiler, func_type);

	for (int i = 0; i < param_count; i++) {
		pop(compiler->owner->current_module_record); // param_types[i]
	}
	pop(compiler->owner->current_module_record); // annotated_return_type
	pop(compiler->owner->current_module_record); // fn
}
