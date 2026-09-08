#include "compiler/compiler_match.h"
#include "compiler/compiler_expressions.h"
#include "compiler/compiler_helpers.h"
#include "compiler/compiler_statements.h"
#include "panic.h"

static MatchCompiler *current_match_compiler(Compiler *compiler)
{
	return &compiler->match_compiler[compiler->match_depth - 1];
}

static void begin_match_scope(Compiler *compiler)
{
	if (compiler->match_depth + 1 == MATCH_NEST_DEPTH)
		compiler_panic(compiler->parser, "Match statement nesting depth exceeded.", SYNTAX);
	compiler->match_depth++;
}

static void end_match_scope(Compiler *compiler)
{
	compiler->match_depth--;
}

static void init_match_compiler(Compiler *compiler, MatchCompiler *match_compiler)
{
	match_compiler->patterns = ALLOCATE(compiler->owner, MatchPattern, 8);
	match_compiler->pattern_count = 0;
	match_compiler->pattern_capacity = 8;
	match_compiler->exhaustiveness.resultant_type = NULL;
	match_compiler->exhaustiveness.has_literal = false;
	match_compiler->exhaustiveness.has_type = false;
	match_compiler->exhaustiveness.has_default = false;
	match_compiler->exhaustiveness.has_err = false;
	match_compiler->exhaustiveness.has_ok = false;
	match_compiler->exhaustiveness.has_some = false;
	match_compiler->exhaustiveness.has_none = false;
	match_compiler->exhaustiveness.matched_types = NULL;
	match_compiler->exhaustiveness.matched_types_count = 0;
	match_compiler->exhaustiveness.matched_types_capacity = 0;
	match_compiler->exhaustiveness.target_type = NULL;
}

static void add_exhaustive_match_type(Compiler *compiler, MatchExhaustiveness *exhaustiveness, ObjectTypeRecord *type)
{
	if (type == NULL)
		return;

	for (int i = 0; i < exhaustiveness->matched_types_count; i++) {
		if (types_equal(exhaustiveness->matched_types[i], type)) {
			return;
		}
	}

	if (exhaustiveness->matched_types_count + 1 > exhaustiveness->matched_types_capacity) {
		const int old_capacity = exhaustiveness->matched_types_capacity;
		exhaustiveness->matched_types_capacity = GROW_CAPACITY(old_capacity);
		exhaustiveness->matched_types = GROW_ARRAY(compiler->owner, ObjectTypeRecord *, exhaustiveness->matched_types,
												   old_capacity, exhaustiveness->matched_types_capacity);
	}

	exhaustiveness->matched_types[exhaustiveness->matched_types_count++] = type;
}

static void record_type_pattern_coverage(Compiler *compiler, MatchExhaustiveness *exhaustiveness,
										 TypeMask matched_type_mask)
{
	ObjectTypeRecord *target_type = exhaustiveness->target_type;
	if (target_type == NULL)
		return;

	if (target_type->base_type == UNION_TYPE) {
		for (int i = 0; i < target_type->as.union_type.element_count; i++) {
			ObjectTypeRecord *element = target_type->as.union_type.element_types[i];
			if (mask_in_type(element, matched_type_mask)) {
				add_exhaustive_match_type(compiler, exhaustiveness, element);
			}
		}
		return;
	}

	if (mask_in_type(target_type, matched_type_mask)) {
		add_exhaustive_match_type(compiler, exhaustiveness, target_type);
	}
}

static bool is_type_match_exhaustive(const MatchExhaustiveness *exhaustiveness)
{
	if (exhaustiveness->target_type == NULL)
		return false;

	if (exhaustiveness->target_type->base_type == UNION_TYPE) {
		return exhaustiveness->matched_types_count >= exhaustiveness->target_type->as.union_type.element_count;
	}

	return exhaustiveness->matched_types_count > 0;
}

static void end_match_binding_scope(Compiler *compiler)
{
	compiler->scope_depth--;
	while (compiler->local_count > 0 && compiler->locals[compiler->local_count - 1].depth > compiler->scope_depth) {
		if (compiler->locals[compiler->local_count - 1].is_captured) {
			compiler_panic(compiler->parser, "Match binding variables cannot be captured.", SYNTAX);
			break;
		}
		compiler->local_count--;
	}
}

static void new_match_expression(Compiler *compiler, const bool can_assign)
{
	(void)can_assign;
	begin_match_scope(compiler);

	expression(compiler); // compile the target

	ObjectTypeRecord *target_type = pop_type_record(compiler);
	MatchCompiler *match_compiler = current_match_compiler(compiler);
	init_match_compiler(compiler, match_compiler);
	if (!target_type)
		target_type = T_ANY;

	MatchExhaustiveness *exhaustiveness = &match_compiler->exhaustiveness;
	exhaustiveness->target_type = target_type;
	exhaustiveness->resultant_type = NULL;

	consume(compiler, CRUX_TOKEN_LEFT_BRACE, "Expected '{' after match target.");
	emit_word(compiler, OP_MATCH);

	match_compiler->end_jumps = ALLOCATE(compiler->owner, int, 8);
	match_compiler->jump_count = 0;
	match_compiler->jump_capacity = 8;

	while (!check(compiler, CRUX_TOKEN_RIGHT_BRACE) && !check(compiler, CRUX_TOKEN_EOF)) {
		MatchPattern pattern = {.jump_if_not_match = -1,
								.binding_slot = UINT16_MAX,
								.is_binding = false,
								.type = MATCH_PATTERN_DEFAULT,
								.type_produced = NULL};
		compiler->last_give_type = NULL;

		if (match(compiler, CRUX_TOKEN_DEFAULT)) {
			pattern.type = MATCH_PATTERN_DEFAULT;
			if (exhaustiveness->has_default) {
				compiler_panic(compiler->parser, "Match expression cannot have multiple 'default' arms.", SYNTAX);
			}
			exhaustiveness->has_default = true;
		} else if (match(compiler, CRUX_TOKEN_OK)) {
			pattern.type = MATCH_PATTERN_OK;

			if (exhaustiveness->has_ok) {
				compiler_panic(compiler->parser, "Match expression cannot have multiple 'Ok' arms.", SYNTAX);
			}
			exhaustiveness->has_ok = true;

			if (target_type->base_type != RESULT_TYPE && target_type->base_type != ANY_TYPE) {
				char buf[256];
				type_record_name(target_type, buf, sizeof(buf));
				compiler_panicf(compiler->parser, TYPE,
								"'Ok' pattern require a match target of type 'Result' got '%s'.", buf);
			}

			pattern.jump_if_not_match = emit_jump(compiler, OP_RESULT_MATCH_OK);

			if (match(compiler, CRUX_TOKEN_LEFT_PAREN)) {
				consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected binding variable name.");
				if (!(compiler->parser->previous.length == 1 && compiler->parser->previous.start[0] == '_')) {
					begin_scope(compiler);
					pattern.is_binding = true;
					declare_variable(compiler);
					pattern.binding_slot = compiler->local_count - 1;
					ObjectTypeRecord *ok_type = target_type->base_type == RESULT_TYPE
													? target_type->as.result_type.ok_type
													: T_ANY;
					compiler->locals[pattern.binding_slot].type = ok_type;
					mark_initialized(compiler);
				}
				consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' to end pattern binding.");
			}

		} else if (match(compiler, CRUX_TOKEN_ERR)) {
			pattern.type = MATCH_PATTERN_ERR;
			if (exhaustiveness->has_err) {
				compiler_panic(compiler->parser, "'match' expression cannot have multiple 'Err' arms.", SYNTAX);
			}
			exhaustiveness->has_err = true;

			if (target_type->base_type != RESULT_TYPE && target_type->base_type != ANY_TYPE) {
				char buf[256];
				type_record_name(target_type, buf, sizeof(buf));
				compiler_panicf(compiler->parser, TYPE,
								"'Err' pattern requires a match target of type 'Result' got '%s'.", buf);
			}

			pattern.jump_if_not_match = emit_jump(compiler, OP_RESULT_MATCH_ERR);

			if (match(compiler, CRUX_TOKEN_LEFT_PAREN)) {
				consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected binding variable name");
				if (!(compiler->parser->previous.length == 1 && compiler->parser->previous.start[0] == '_')) {
					begin_scope(compiler);
					pattern.is_binding = true;
					declare_variable(compiler);
					pattern.binding_slot = compiler->local_count - 1;
					compiler->locals[pattern.binding_slot].type = T_ERROR;
					mark_initialized(compiler);
				}
				consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' to end pattern binding.");
			}

		} else if (match(compiler, CRUX_TOKEN_SOME)) {
			pattern.type = MATCH_PATTERN_SOME;
			if (exhaustiveness->has_some) {
				compiler_panic(compiler->parser, "'match' expression cannot have multiple 'Some' arms.", TYPE);
			}
			exhaustiveness->has_some = true;

			if (target_type->base_type != OPTION_TYPE && target_type->base_type != ANY_TYPE) {
				char buf[256];
				type_record_name(target_type, buf, sizeof(buf));
				compiler_panicf(compiler->parser, TYPE,
								"'Some' pattern requires a match target of type 'Option' got '%s'.", buf);
			}

			pattern.jump_if_not_match = emit_jump(compiler, OP_OPTION_MATCH_SOME);

			if (match(compiler, CRUX_TOKEN_LEFT_PAREN)) {
				consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected binding variable name");
				if (!(compiler->parser->previous.length == 1 && compiler->parser->previous.start[0] == '_')) {
					begin_scope(compiler);
					pattern.is_binding = true;
					declare_variable(compiler);
					pattern.binding_slot = compiler->local_count - 1;

					compiler->locals[pattern.binding_slot].type = target_type->base_type == OPTION_TYPE
																	  ? target_type->as.option_type.some_type
																	  : T_ANY;

					mark_initialized(compiler);
				}
				consume(compiler, CRUX_TOKEN_RIGHT_PAREN, "Expected ')' to end pattern binding.");
			}

		} else if (match(compiler, CRUX_TOKEN_NONE)) {
			pattern.type = MATCH_PATTERN_NONE;
			if (exhaustiveness->has_none) {
				compiler_panic(compiler->parser, "'match' expression cannot have multiple 'None' arms.", SYNTAX);
			}
			exhaustiveness->has_none = true;

			if (target_type->base_type != OPTION_TYPE && target_type->base_type != ANY_TYPE) {
				char buf[256];
				type_record_name(target_type, buf, sizeof(buf));
				compiler_panicf(compiler->parser, TYPE,
								"'None' pattern requires a match target of type 'Option' got '%s'.", buf);
			}

			pattern.jump_if_not_match = emit_jump(compiler, OP_OPTION_MATCH_NONE);

		} else if (match_type_name(compiler)) {
			pattern.type = MATCH_PATTERN_TYPE;
			exhaustiveness->has_type = true;
			Token type_token = compiler->parser->previous;
			TypeMask matched_type_mask = type_token_type_to_mask(type_token.type);

			bool target_has_type = mask_in_type(target_type, matched_type_mask);
			if (!target_has_type) {
				char buf[TYPE_NAME_BUF_SIZE];
				type_mask_name(matched_type_mask, buf, sizeof(buf));
				char target_buf[TYPE_NAME_BUF_SIZE];
				type_record_name(target_type, target_buf, sizeof(target_buf));
				compiler_panicf(compiler->parser, TYPE, "Cannot match '%s' because it is not included in '%s'", buf,
								target_buf);
			}

			emit_word(compiler, OP_TYPE_MATCH);
			emit_word(compiler, (uint16_t)matched_type_mask);
			pattern.jump_if_not_match = current_chunk(compiler)->count;
			emit_word(compiler, 0xffff);
			record_type_pattern_coverage(compiler, exhaustiveness, matched_type_mask);

		} else {
			pattern.type = MATCH_PATTERN_EXPRESSION;
			exhaustiveness->has_literal = true;
			expression(compiler);
			ObjectTypeRecord *pattern_type = pop_type_record(compiler);

			if (pattern_type && target_type && pattern_type->base_type != ANY_TYPE &&
				target_type->base_type != ANY_TYPE) {
				if (!types_compatible(target_type, pattern_type)) {
					char expected[128], got[128];
					type_record_name(target_type, expected, sizeof(expected));
					type_record_name(pattern_type, got, sizeof(got));
					compiler_panicf(compiler->parser, TYPE,
									"Pattern type '%s' is not compatible with match target type '%s'.", got, expected);
				}
			}

			pattern.jump_if_not_match = emit_jump(compiler, OP_MATCH_JUMP);
		}

		consume(compiler, CRUX_TOKEN_EQUAL_ARROW, "Expected '=>' after pattern.");

		if ((pattern.type == MATCH_PATTERN_ERR || pattern.type == MATCH_PATTERN_OK ||
			 pattern.type == MATCH_PATTERN_SOME) &&
			pattern.binding_slot != UINT16_MAX) {
			emit_words(compiler, OP_RESULT_BIND, pattern.binding_slot);
		}

		if (match(compiler, CRUX_TOKEN_LEFT_BRACE)) {
			block(compiler);
			// blocks produce values with `give VALUE` and statement (e.g. return, break ...)
			pattern.type_produced = compiler->last_give_type ? compiler->last_give_type : T_NIL;
		} else if (match(compiler, CRUX_TOKEN_GIVE)) {
			if (match(compiler, CRUX_TOKEN_NIL)) {
				emit_word(compiler, OP_NIL);
				pattern.type_produced = T_NIL;
				consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after give expression");
			} else {
				expression(compiler);
				pattern.type_produced = pop_type_record(compiler);
				consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after give expression");
			}
			emit_word(compiler, OP_GIVE);
		} else {
			expression(compiler);
			pattern.type_produced = pop_type_record(compiler);
			consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after expression.");
		}

		if (exhaustiveness->resultant_type == NULL || exhaustiveness->resultant_type->base_type == NEVER_TYPE) {
			exhaustiveness->resultant_type = pattern.type_produced;
		} else if (pattern.type_produced->base_type == NEVER_TYPE) {
			// This arm produces never, so it does affect the resultant type
		} else if (exhaustiveness->resultant_type->base_type != ANY_TYPE &&
				   pattern.type_produced->base_type != ANY_TYPE) {
			if (!types_compatible(pattern.type_produced, exhaustiveness->resultant_type)) {
				char expected[TYPE_NAME_BUF_SIZE], got[TYPE_NAME_BUF_SIZE];
				type_record_name(exhaustiveness->resultant_type, expected, sizeof(expected));
				type_record_name(pattern.type_produced, got, sizeof(got));
				compiler_panicf(compiler->parser, TYPE,
								"Match arms produce inconsistent types: expected '%s', got '%s'.", expected, got);
			}
		}

		if (pattern.is_binding) {
			end_match_binding_scope(compiler);
		}

		if (match_compiler->jump_count + 1 > match_compiler->jump_capacity) {
			const int old_cap = match_compiler->jump_capacity;
			match_compiler->jump_capacity = GROW_CAPACITY(match_compiler->jump_capacity);
			match_compiler->end_jumps = GROW_ARRAY(compiler->owner, int, match_compiler->end_jumps, old_cap,
												   match_compiler->jump_capacity);
		}

		match_compiler->end_jumps[match_compiler->jump_count++] = emit_jump(compiler, OP_JUMP);

		if (pattern.jump_if_not_match != -1) {
			patch_jump(compiler, pattern.jump_if_not_match);
		}

		if (match_compiler->pattern_count + 1 > match_compiler->pattern_capacity) {
			const int old_cap = match_compiler->pattern_capacity;
			match_compiler->pattern_capacity = GROW_CAPACITY(match_compiler->pattern_capacity);
			match_compiler->patterns = GROW_ARRAY(compiler->owner, MatchPattern, match_compiler->patterns, old_cap,
												  match_compiler->pattern_capacity);
		}

		match_compiler->patterns[match_compiler->pattern_count++] = pattern;
	}

	if (match_compiler->jump_count == 0) {
		compiler_panic(compiler->parser, "'match' expression must have at least one arm.", SYNTAX);
	}

	// Exhaustiveness checks

	const bool has_ok_err = exhaustiveness->has_ok && exhaustiveness->has_err;
	const bool has_some_none = exhaustiveness->has_some && exhaustiveness->has_none;

	if ((exhaustiveness->has_ok || exhaustiveness->has_err) && !has_ok_err && !exhaustiveness->has_default) {
		compiler_panic(compiler->parser,
					   "Result match expression must have both 'Ok' and 'Err' arms or a 'default' arm.", SYNTAX);
	}
	if ((exhaustiveness->has_some || exhaustiveness->has_none) && !has_some_none && !exhaustiveness->has_default) {
		compiler_panic(compiler->parser,
					   "Option match expression must have both 'Some' and 'None' arms or a 'default' arm.", SYNTAX);
	}
	if (exhaustiveness->has_type && !exhaustiveness->has_default && !is_type_match_exhaustive(exhaustiveness)) {
		char target_buf[TYPE_NAME_BUF_SIZE];
		type_record_name(target_type, target_buf, sizeof(target_buf));
		compiler_panicf(compiler->parser, SYNTAX,
						"Type match expression over '%s' must cover every target type or include a 'default' arm.",
						target_buf);
	}
	if (!exhaustiveness->has_default && !has_ok_err && !has_some_none && !exhaustiveness->has_type) {
		compiler_panic(compiler->parser, "'match' expression must have a 'default' case.", SYNTAX);
	}

	for (int i = 0; i < match_compiler->jump_count; i++) {
		patch_jump(compiler, match_compiler->end_jumps[i]);
	}

	emit_word(compiler, OP_MATCH_END);
	consume(compiler, CRUX_TOKEN_RIGHT_BRACE, "Expected '}' to end match expression.");
	push_type_record(compiler, match_compiler->exhaustiveness.resultant_type
								   ? match_compiler->exhaustiveness.resultant_type
								   : T_ANY);
	end_match_scope(compiler);
	FREE_ARRAY(compiler->owner, MatchPattern, match_compiler->patterns, match_compiler->pattern_capacity);
	FREE_ARRAY(compiler->owner, int, match_compiler->end_jumps, match_compiler->jump_capacity);
	FREE_ARRAY(compiler->owner, ObjectTypeRecord *, exhaustiveness->matched_types,
			   exhaustiveness->matched_types_capacity);
}

void match_expression(Compiler *compiler, const bool can_assign)
{
	new_match_expression(compiler, can_assign);
}
