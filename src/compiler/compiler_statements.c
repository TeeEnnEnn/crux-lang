#include <string.h>

#include "compiler/compiler_declarations.h"
#include "compiler/compiler_expressions.h"
#include "compiler/compiler_helpers.h"
#include "compiler/compiler_statements.h"
#include "file_handler.h"
#include "panic.h"

void expression_statement(Compiler *compiler)
{
	expression(compiler);
	pop_type_record(compiler); // discard — value is unused
	consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after expression.");
	emit_word(compiler, OP_POP);
}

void while_statement(Compiler *compiler)
{
	begin_scope(compiler);
	const int loopStart = current_chunk(compiler)->count;
	push_loop_context(compiler, LOOP_WHILE, loopStart);

	expression(compiler);

	ObjectTypeRecord *condition_type = pop_type_record(compiler);
	if (condition_type && condition_type->base_type != BOOL_TYPE && condition_type->base_type != ANY_TYPE) {
		char got[128];
		type_record_name(condition_type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "'while' condition must be of type 'Bool', got '%s'.", got);
	}

	const int exitJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
	emit_word(compiler, OP_POP);
	statement(compiler);
	emit_loop(compiler, loopStart);
	patch_jump(compiler, exitJump);
	emit_word(compiler, OP_POP);

	pop_loop_context(compiler);
	end_scope(compiler);
}

void for_in(Compiler *compiler, const bool declares_binding)
{
	consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected loop variable name.");
	const Token binding_name = compiler->parser->previous;
	consume(compiler, CRUX_TOKEN_IN, "Expected 'in' after loop variable.");

	ObjectTypeRecord *binding_type = T_ANY;
	uint16_t set_op = OP_SET_LOCAL;
	int target_arg = -1;
	int iterator_slot = -1;

	// Create a hidden iterator variable
	static const char hidden_iter_name[] = "#for_iter";
	const Token iterator_name = {.type = CRUX_TOKEN_IDENTIFIER,
								 .start = hidden_iter_name,
								 .length = (int)(sizeof(hidden_iter_name) - 1),
								 .line = binding_name.line};
	add_local(compiler, iterator_name, T_ANY);
	iterator_slot = compiler->local_count - 1;

	expression(compiler);
	const ObjectTypeRecord *iterable_type = pop_type_record(compiler);
	binding_type = get_iterable_element_type(compiler, iterable_type);
	ObjectTypeRecord *iterator_type = new_iterator_type_rec(compiler->owner, binding_type);

	emit_word(compiler, OP_ITER_INIT);
	compiler->locals[iterator_slot].type = iterator_type;
	mark_initialized(compiler);

	if (declares_binding) {
		declare_named_variable(compiler, binding_name, binding_type);
		target_arg = compiler->local_count - 1;
		emit_word(compiler, OP_NIL);
		mark_initialized(compiler);
	} else {
		ObjectTypeRecord *target_type = NULL;
		if (!resolve_assignment_target(compiler, binding_name, &set_op, &target_arg, &target_type)) {
			return;
		}

		if (target_type && target_type->base_type != ANY_TYPE && binding_type->base_type != ANY_TYPE &&
			!types_compatible(target_type, binding_type)) {
			char expected[128], got[128];
			type_record_name(target_type, expected, sizeof(expected));
			type_record_name(binding_type, got, sizeof(got));
			compiler_panicf(compiler->parser, TYPE, "Cannot assign '%s' to variable of type '%s'.", got, expected);
		}
	}

	const int loop_start = current_chunk(compiler)->count;
	push_loop_context(compiler, LOOP_FOR, loop_start);

	emit_words(compiler, OP_GET_LOCAL, iterator_slot);
	const int exit_jump = emit_jump(compiler, OP_ITER_NEXT);
	emit_words(compiler, set_op, target_arg);
	emit_word(compiler, OP_POP);

	statement(compiler);
	emit_loop(compiler, loop_start);

	patch_jump(compiler, exit_jump);
	pop_loop_context(compiler);
}

void c_style_for(Compiler *compiler)
{
	int loopStart = current_chunk(compiler)->count;
	int exitJump = -1;

	if (!match(compiler, CRUX_TOKEN_SEMICOLON)) { // if there is no semicolon, there is a condition
		expression(compiler);

		// Condition must be Bool
		const ObjectTypeRecord *condition_type = pop_type_record(compiler);
		if (condition_type && condition_type->base_type != BOOL_TYPE && condition_type->base_type != ANY_TYPE) {
			char got[128];
			type_record_name(condition_type, got, sizeof(got));
			compiler_panicf(compiler->parser, TYPE, "'for' condition must be of type 'Bool', got '%s'.", got);
		}

		consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after loop condition");
		exitJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
		emit_word(compiler, OP_POP);
	}

	const int bodyJump = emit_jump(compiler, OP_JUMP);
	const int incrementStart = current_chunk(compiler)->count;
	push_loop_context(compiler, LOOP_FOR, incrementStart);

	// no type enforcement, but pop type
	expression(compiler);
	pop_type_record(compiler);
	emit_word(compiler, OP_POP);

	emit_loop(compiler, loopStart);
	loopStart = incrementStart;
	patch_jump(compiler, bodyJump);

	statement(compiler);
	emit_loop(compiler, loopStart);

	if (exitJump != -1) {
		patch_jump(compiler, exitJump);
		emit_word(compiler, OP_POP);
	}

	pop_loop_context(compiler);
}

void for_statement(Compiler *compiler)
{
	begin_scope(compiler);

	if (check(compiler, CRUX_TOKEN_LET)) {
		advance(compiler);
		if (check(compiler, CRUX_TOKEN_IDENTIFIER) && peek_next_token(compiler).type == CRUX_TOKEN_IN) {
			for_in(compiler, true);
			end_scope(compiler);
			return;
		}

		var_declaration(compiler, false);
		c_style_for(compiler);
		end_scope(compiler);
		return;
	}

	if (check(compiler, CRUX_TOKEN_IDENTIFIER) && peek_next_token(compiler).type == CRUX_TOKEN_IN) {
		for_in(compiler, false);
		end_scope(compiler);
		return;
	}

	if (match(compiler, CRUX_TOKEN_SEMICOLON)) {
		// no initializer
	} else {
		expression_statement(compiler);
	}
	c_style_for(compiler);
	end_scope(compiler);
}

void if_statement(Compiler *compiler)
{
	compiler->current_narrowing.tracked_local_index = -1;
	compiler->current_narrowing.tracked_global_name = NULL;
	compiler->current_narrowing.tracked_is_typeof = false;
	compiler->current_narrowing.tracked_literal_type = NULL;
	compiler->current_narrowing.local_index = -1;
	compiler->current_narrowing.global_name = NULL;
	compiler->current_narrowing.narrowed_to = NULL;
	compiler->current_narrowing.stripped_down = NULL;

	expression(compiler);

	const NarrowingInfo narrow_state = compiler->current_narrowing;
	ObjectTypeRecord *original_type = NULL;

	// Condition must be Bool
	const ObjectTypeRecord *condition_type = pop_type_record(compiler);
	if (condition_type && condition_type->base_type != BOOL_TYPE && condition_type->base_type != ANY_TYPE) {
		compiler_panic(compiler->parser, "'if' condition must be of type 'Bool'.", TYPE);
	}

	const int thenJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
	emit_word(compiler, OP_POP);

	// then block narrowing
	if (narrow_state.narrowed_to) {
		if (narrow_state.local_index != -1) {
			original_type = compiler->locals[narrow_state.local_index].type;
			compiler->locals[narrow_state.local_index].type = narrow_state.narrowed_to;
		} else if (narrow_state.global_name) {
			type_table_get(compiler->type_table, narrow_state.global_name, &original_type);
			if (original_type != NULL) {
				type_table_set(compiler->type_table, narrow_state.global_name, narrow_state.narrowed_to);
			}
		}
	}

	push(compiler->owner->current_module_record, original_type ? OBJECT_VAL(original_type) : NIL_VAL);

	statement(compiler);

	const int elseJump = emit_jump(compiler, OP_JUMP);
	patch_jump(compiler, thenJump);
	emit_word(compiler, OP_POP);

	// apply stripped down type for else
	if (narrow_state.stripped_down) {
		if (narrow_state.local_index != -1) {
			compiler->locals[narrow_state.local_index].type = narrow_state.stripped_down;
		} else if (narrow_state.global_name) {
			type_table_set(compiler->type_table, narrow_state.global_name, narrow_state.stripped_down);
		}
	} else {
		if (narrow_state.local_index != -1) {
			compiler->locals[narrow_state.local_index].type = original_type;
		} else if (narrow_state.global_name && original_type != NULL) {
			type_table_set(compiler->type_table, narrow_state.global_name, original_type);
		}
	}

	if (match(compiler, CRUX_TOKEN_ELSE)) {
		statement(compiler);
	}

	patch_jump(compiler, elseJump);

	pop(compiler->owner->current_module_record); // original_type

	// restore original type
	if (narrow_state.local_index != -1) {
		compiler->locals[narrow_state.local_index].type = original_type;
	} else if (narrow_state.global_name && original_type != NULL) {
		type_table_set(compiler->type_table, narrow_state.global_name, original_type);
	}
}

void return_statement(Compiler *compiler)
{
	if (compiler->type == TYPE_SCRIPT) {
		compiler_panic(compiler->parser, "Cannot use <return> outside of a function.", SYNTAX);
	}

	compiler->has_return = true;

	if (match(compiler, CRUX_TOKEN_SEMICOLON)) {
		// check that the function expects Nil
		if (compiler->return_type && compiler->return_type->base_type != NIL_TYPE &&
			compiler->return_type->base_type != ANY_TYPE) {
			compiler_panic(compiler->parser, "Non-Nil return type requires a return value.", TYPE);
		}
		emit_return(compiler);
	} else {
		expression(compiler);
		consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after return value.");

		ObjectTypeRecord *value_type = pop_type_record(compiler);

		// Only validate if both sides are known
		if (value_type && compiler->return_type && compiler->return_type->base_type != ANY_TYPE &&
			value_type->base_type != ANY_TYPE) {
			if (!types_compatible(compiler->return_type, value_type)) {
				char expected[128];
				char got[128];
				type_record_name(compiler->return_type, expected, sizeof(expected));
				type_record_name(value_type, got, sizeof(got));
				compiler_panicf(compiler->parser, TYPE, "Return type mismatch: expected '%s', got '%s'.", expected,
								got);
			}
		}
		emit_word(compiler, OP_RETURN);
	}
	compiler->last_give_type = new_type_rec(compiler->owner, NEVER_TYPE);
}

void use_statement(Compiler *compiler, bool is_public)
{
	bool hasParen = false;
	if (compiler->parser->current.type == CRUX_TOKEN_LEFT_PAREN) {
		consume(compiler, CRUX_TOKEN_LEFT_PAREN, "Expected '(' after use statement.");
		hasParen = true;
	}

	uint16_t nameCount = 0;
	bool aliasPresence[UINT8_MAX] = {0};
	Token nameTokens[UINT8_MAX] = {0};
	Token aliasTokens[UINT8_MAX] = {0};

	do {
		if (nameCount >= UINT8_MAX) {
			compiler_panicf(compiler->parser, IMPORT, "Cannot import more than %d names.", UINT8_MAX);
		}
		consume_identifier_like(compiler, "Expected name to import from module.");

		nameTokens[nameCount] = compiler->parser->previous;

		if (compiler->parser->current.type == CRUX_TOKEN_AS) {
			consume(compiler, CRUX_TOKEN_AS, "Expected 'as' keyword.");
			consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected alias name after 'as'.");
			aliasTokens[nameCount] = compiler->parser->previous;
			aliasPresence[nameCount] = true;
		}

		nameCount++;
	} while (match(compiler, CRUX_TOKEN_COMMA));

	if (hasParen) {
		consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' after last imported name.");
	}

	consume(compiler, CRUX_TOKEN_FROM, "Expected 'from' after import statement.");
	consume(compiler, CRUX_TOKEN_STRING, "Expected string literal for module name.");

	const bool is_native = memcmp(compiler->parser->previous.start, "\"crux:", 6) == 0;
	const bool is_file = !is_native;

	if (is_native) {
		ObjectString *native_module_name = copy_string(compiler->owner, compiler->parser->previous.start + 6,
													   compiler->parser->previous.length - 7);

		NativeModule *module = NULL;
		for (int i = 0; i < compiler->owner->native_modules.capacity; i++) {
			if (native_module_name == compiler->owner->native_modules.modules[i].name) {
				module = &compiler->owner->native_modules.modules[i];
				break;
			}
		}

		if (module == NULL) {
			compiler_panicf(compiler->parser, NAME, "Failed to find native module '%s'.", native_module_name->chars);
			return;
		}

		for (int i = 0; i < nameCount; i++) {
			Token name_tok = nameTokens[i];
			Token alias_tok = aliasPresence[i] ? aliasTokens[i] : name_tok;

			ObjectString *real_name = copy_string(compiler->owner, name_tok.start, name_tok.length);
			ObjectString *alias_name = copy_string(compiler->owner, alias_tok.start, alias_tok.length);

			CruxValue callable_value;
			if (!table_get(module->names, real_name, &callable_value)) {
				compiler_panicf(compiler->parser, NAME, "Failed to find name '%s' in module '%s'.", real_name->chars,
								native_module_name->chars);
				return;
			}

			uint16_t const_index = make_constant(compiler, callable_value);

			ObjectNativeCallable *native_callable = AS_CRUX_NATIVE_CALLABLE(callable_value);
			ObjectTypeRecord **args_copy = NULL;
			if (native_callable->arity > 0) {
				args_copy = ALLOCATE(compiler->owner, ObjectTypeRecord *, native_callable->arity);
				for (int k = 0; k < native_callable->arity; k++)
					args_copy[k] = native_callable->arg_types[k];
			}
			ObjectTypeRecord *resolved_type = new_function_type_rec(compiler->owner, args_copy, native_callable->arity,
																	native_callable->return_type);

			if (compiler->scope_depth > 0) {
				if (is_public) {
					compiler_panic(compiler->parser, "Cannot use 'pub' on local imports.", SYNTAX);
				}
				add_local(compiler, alias_tok, resolved_type);
				mark_initialized(compiler);
				emit_words(compiler, OP_CONSTANT, const_index);
			} else {
				int global_index = compiler->global_count++;
				table_set(compiler->owner, &compiler->globals, alias_name, INT_VAL(global_index));
				type_table_set(compiler->type_table, alias_name, resolved_type);
				if (is_public && compiler->owner->current_module_record != NULL) {
					type_table_set(compiler->owner->current_module_record->types, alias_name, resolved_type);
				}

				emit_words(compiler, OP_CONSTANT, const_index);
				emit_words(compiler, is_public ? OP_DEFINE_PUB_GLOBAL : OP_DEFINE_GLOBAL, global_index);
			}
		}
	}

	if (is_file) {
		ObjectString *raw_path_str = copy_string(compiler->owner, compiler->parser->previous.start + 1,
												 compiler->parser->previous.length - 2);

		push(compiler->owner->current_module_record, OBJECT_VAL(raw_path_str));

		const char *base_path = compiler->owner->current_module_record && compiler->owner->current_module_record->path
									? compiler->owner->current_module_record->path->chars
									: ".";

		char *resolved_chars = NULL;
		if (compiler->owner->config.resolveModuleFn) {
			const char* resolved = compiler->owner->config.resolveModuleFn(compiler->owner, base_path, raw_path_str->chars);
			if (resolved) resolved_chars = strdup(resolved);
		}

		if (resolved_chars == NULL) {
			resolved_chars = resolve_path(base_path, raw_path_str->chars);
		}

		if (resolved_chars == NULL) {
			compiler_panicf(compiler->parser, IMPORT, "Failed to resolve import path: '%s'", raw_path_str->chars);
			pop(compiler->owner->current_module_record);
			return;
		}

		ObjectString *path_str = copy_string(compiler->owner, resolved_chars, strlen(resolved_chars));
		free(resolved_chars);

		pop(compiler->owner->current_module_record); // raw_path_str
		push(compiler->owner->current_module_record, OBJECT_VAL(path_str));

		ObjectModuleRecord *statically_imported_mod = NULL;

		statically_imported_mod = compile_module_statically(compiler, path_str);
		if (!statically_imported_mod || statically_imported_mod->state == STATE_ERROR) {
			compiler_panicf(compiler->parser, IMPORT, "Failed to compile module '%s'.", path_str->chars);
			pop(compiler->owner->current_module_record); // path_str
			return;
		}

		// these opcodes execute the module
		uint16_t module_const = make_constant(compiler, OBJECT_VAL(path_str));
		emit_words(compiler, OP_USE_MODULE, module_const);
		emit_words(compiler, is_public ? OP_FINISH_PUB_USE : OP_FINISH_USE, nameCount);

		// resolve types and emit indexes
		for (uint16_t i = 0; i < nameCount; i++) {
			Token name_tok = nameTokens[i];
			Token alias_tok = aliasPresence[i] ? aliasTokens[i] : name_tok;

			ObjectString *real_name = copy_string(compiler->owner, name_tok.start, name_tok.length);
			ObjectString *alias_name = copy_string(compiler->owner, alias_tok.start, alias_tok.length);

			ObjectTypeRecord *resolved_type = NULL;
			if (statically_imported_mod && !type_table_get(statically_imported_mod->types, real_name, &resolved_type)) {
				compiler_panicf(compiler->parser, NAME, "Module does not export '%s'", real_name->chars);
				resolved_type = T_ANY; // Fallback
			} else if (!statically_imported_mod) {
				resolved_type = T_ANY; // Dynamic imports don't know the type
			}

			// Emit the original name so the CruxVM can look it up in the module's `publics`
			uint16_t original_name_const = make_constant(compiler, OBJECT_VAL(real_name));
			emit_word(compiler, original_name_const);

			if (compiler->scope_depth > 0) {
				if (is_public) {
					compiler_panic(compiler->parser, "Cannot use 'pub' on local imports.", SYNTAX);
				}
				add_local(compiler, alias_tok, resolved_type);
				mark_initialized(compiler);

				// 0xFFFF sentinel - leaves value on stack
				emit_word(compiler, 0xFFFF);
			} else {
				int global_index = compiler->global_count++;
				table_set(compiler->owner, &compiler->globals, alias_name, INT_VAL(global_index));
				type_table_set(compiler->type_table, alias_name, resolved_type);
				if (is_public && compiler->owner->current_module_record != NULL) {
					type_table_set(compiler->owner->current_module_record->types, alias_name, resolved_type);
				}

				emit_word(compiler, global_index);
			}
		}

		pop(compiler->owner->current_module_record); // path_str
	}

	consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after import statement.");
}

void continue_statement(Compiler *compiler)
{
	consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after 'continue',");
	const int continueTarget = get_current_continue_target(compiler);
	if (continueTarget == -1) {
		return;
	}
	const LoopContext *loopContext = &compiler->loop_stack[compiler->loop_depth - 1];
	emit_cleanup_for_jump(compiler, loopContext->scope_depth);
	emit_loop(compiler, continueTarget);
	compiler->last_give_type = new_type_rec(compiler->owner, NEVER_TYPE);
}

void break_statement(Compiler *compiler)
{
	consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after 'break'.");
	if (compiler->loop_depth <= 0) {
		compiler_panic(compiler->parser, "Cannot use 'break' outside of a loop.", SYNTAX);
		return;
	}
	const LoopContext *loopContext = &compiler->loop_stack[compiler->loop_depth - 1];
	emit_cleanup_for_jump(compiler, loopContext->scope_depth);
	add_break_jump(compiler, emit_jump(compiler, OP_JUMP));
	compiler->last_give_type = new_type_rec(compiler->owner, NEVER_TYPE);
}

void panic_statement(Compiler *compiler)
{
	expression(compiler);
	const ObjectTypeRecord *type = pop_type_record(compiler);

	if (type && type->base_type != STRING_TYPE && type->base_type != ANY_TYPE) {
		char got[128];
		type_record_name(type, got, sizeof(got));
		compiler_panicf(compiler->parser, TYPE, "'panic' requires a 'String', got '%s'.", got);
	}
	consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after 'panic'.");
	emit_word(compiler, OP_PANIC);

	compiler->last_give_type = new_type_rec(compiler->owner, NEVER_TYPE);
}

void block(Compiler *compiler)
{
	while (!check(compiler, CRUX_TOKEN_RIGHT_BRACE) && !check(compiler, CRUX_TOKEN_EOF)) {
		declaration(compiler);
	}

	consume(compiler, CRUX_TOKEN_RIGHT_BRACE, "Expected '}' after block");
}

void give_statement(Compiler *compiler)
{
	if (compiler->match_depth == 0) {
		compiler_panic(compiler->parser, "'give' can only be used inside a match expression.", SYNTAX);
	}

	if (match(compiler, CRUX_TOKEN_SEMICOLON)) {
		emit_word(compiler, OP_NIL);
		compiler->last_give_type = T_NIL;
	} else {
		expression(compiler);
		// Record for match_expression to check arm consistency.
		compiler->last_give_type = pop_type_record(compiler);
		consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after give statement.");
	}

	emit_word(compiler, OP_GIVE);
}

void statement(Compiler *compiler)
{
	if (match(compiler, CRUX_TOKEN_IF)) {
		if_statement(compiler);
	} else if (match(compiler, CRUX_TOKEN_LEFT_BRACE)) {
		begin_scope(compiler);
		block(compiler);
		end_scope(compiler);
	} else if (match(compiler, CRUX_TOKEN_WHILE)) {
		while_statement(compiler);
	} else if (match(compiler, CRUX_TOKEN_FOR)) {
		for_statement(compiler);
	} else if (match(compiler, CRUX_TOKEN_RETURN)) {
		return_statement(compiler);
	} else if (match(compiler, CRUX_TOKEN_USE)) {
		use_statement(compiler, false);
	} else if (match(compiler, CRUX_TOKEN_GIVE)) {
		give_statement(compiler);
	} else if (match(compiler, CRUX_TOKEN_BREAK)) {
		break_statement(compiler);
	} else if (match(compiler, CRUX_TOKEN_CONTINUE)) {
		continue_statement(compiler);
	} else if (match(compiler, CRUX_TOKEN_PANIC)) {
		panic_statement(compiler);
	} else {
		expression_statement(compiler);
	}
}
