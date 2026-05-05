#include "compiler/compiler_helpers.h"
#include "compiler/compiler_declarations.h"
#include "compiler/compiler_expressions.h"
#include "compiler/compiler_functions.h"
#include "compiler/compiler_statements.h"
#include "panic.h"

void var_declaration(Compiler *compiler, const bool is_public)
{
	const uint16_t global = parse_variable(compiler, "Expected Variable Name.");
	const Token var_name = compiler->parser->previous;
	ObjectString *name_str = copy_string(compiler->owner, var_name.start, var_name.length);

	push(compiler->owner->current_module_record, OBJECT_VAL(name_str));

	ObjectTypeRecord *annotated_type = NULL;
	if (match(compiler, CRUX_TOKEN_COLON)) {
		annotated_type = parse_type_record(compiler);
	}

	push(compiler->owner->current_module_record, annotated_type ? OBJECT_VAL(annotated_type) : NIL_VAL);

	ObjectTypeRecord *value_type = NULL;
	if (match(compiler, CRUX_TOKEN_EQUAL)) {
		expression(compiler);
		value_type = pop_type_record(compiler);

		if (annotated_type && value_type && annotated_type->base_type != ANY_TYPE &&
			value_type->base_type != ANY_TYPE && !types_compatible(annotated_type, value_type)) {
			char expected[128], got[128];
			type_record_name(annotated_type, expected, sizeof(expected));
			type_record_name(value_type, got, sizeof(got));
			compiler_panicf(compiler->parser, TYPE, "Type mismatch: expected '%s', got '%s'.", expected, got);
		}
	} else {
		if (annotated_type && annotated_type->base_type != NIL_TYPE && annotated_type->base_type != ANY_TYPE) {
			compiler_panic(compiler->parser, "Variable with non-Nil type must be initialized.", TYPE);
		}
		emit_word(compiler, OP_NIL);
		value_type = T_NIL;
	}

	ObjectTypeRecord *resolved_type = annotated_type ? annotated_type : value_type;

	if (annotated_type && annotated_type->base_type == SHAPE_TYPE && value_type &&
		value_type->base_type == STRUCT_TYPE) {
		// If the struct perfectly satisfies the shape upgrade to concrete struct
		if (types_compatible(annotated_type, value_type)) {
			resolved_type = value_type;
		}
	}

	if (!resolved_type)
		resolved_type = T_ANY;

	if (is_public || (compiler->owner->current_module_record && compiler->owner->current_module_record->is_repl)) {
		type_table_set(compiler->owner->current_module_record->types, name_str, resolved_type);
	}

	consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

	if (compiler->scope_depth > 0) {
		compiler->locals[compiler->local_count - 1].type = resolved_type;
	} else {
		type_table_set(compiler->type_table, name_str, resolved_type);
	}
	define_variable(compiler, global, is_public);

	pop(compiler->owner->current_module_record); // annotated_type
	pop(compiler->owner->current_module_record); // name_str
}

static void struct_declaration(Compiler *compiler, bool is_public)
{
	consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected struct name.");
	const Token structName = compiler->parser->previous;

	ObjectString *struct_name_str = copy_string(compiler->owner, structName.start, structName.length);
	push(compiler->owner->current_module_record, OBJECT_VAL(struct_name_str));

	const uint16_t nameConstant = identifier_constant(compiler, &structName);
	ObjectStruct *structObject = new_struct_type(compiler->owner, struct_name_str);
	push(compiler->owner->current_module_record, OBJECT_VAL(structObject));

	declare_variable(compiler);

	const int local_index = (compiler->scope_depth > 0) ? compiler->local_count - 1 : -1;

	const uint16_t structConstant = make_constant(compiler, OBJECT_VAL(structObject));
	emit_words(compiler, OP_STRUCT, structConstant);
	define_variable(compiler, nameConstant, is_public);

	consume(compiler, CRUX_TOKEN_LEFT_BRACE, "Expected '{' before struct body.");

	ObjectTypeTable *field_types = new_type_table(compiler->owner, INITIAL_TYPE_TABLE_SIZE);
	push(compiler->owner->current_module_record, OBJECT_VAL(field_types));
	int fieldCount = 0;

	if (!match(compiler, CRUX_TOKEN_RIGHT_BRACE)) {
		do {
			if (fieldCount >= UINT16_MAX) {
				compiler_panic(compiler->parser, "Too many fields in struct.", SYNTAX);
				break;
			}

			consume(compiler, CRUX_TOKEN_IDENTIFIER,
					"Expected field name. Trailing comma after last field is not allowed.");
			ObjectString *fieldName = copy_string(compiler->owner, compiler->parser->previous.start,
												  compiler->parser->previous.length);
			push(compiler->owner->current_module_record, OBJECT_VAL(fieldName));

			CruxValue fieldNameCheck;
			if (table_get(&structObject->fields, fieldName, &fieldNameCheck)) {
				compiler_panic(compiler->parser,
							   "Duplicate field name in struct "
							   "declaration.",
							   SYNTAX);
				break;
			}

			// Optional field type annotation: fieldName: Type
			ObjectTypeRecord *field_type = NULL;
			if (match(compiler, CRUX_TOKEN_COLON)) {
				field_type = parse_type_record(compiler);
			} else {
				field_type = T_ANY;
			}
			push(compiler->owner->current_module_record, OBJECT_VAL(field_type));

			type_table_set(field_types, fieldName, field_type);

			table_set(compiler->owner, &structObject->fields, fieldName, INT_VAL(fieldCount));
			fieldCount++;
		} while (match(compiler, CRUX_TOKEN_COMMA));
	}

	if (fieldCount != 0) {
		consume(compiler, CRUX_TOKEN_RIGHT_BRACE, "Expected '}' after struct body.");
	}

	ObjectTypeRecord *struct_type = new_struct_type_rec(compiler->owner, structObject, field_types, fieldCount);
	push(compiler->owner->current_module_record, OBJECT_VAL(struct_type));

	// type registration
	if (compiler->scope_depth == 0) {
		type_table_set(compiler->type_table, struct_name_str, struct_type);
	} else {
		compiler->locals[local_index].type = struct_type;
	}
	if (is_public || (compiler->owner->current_module_record && compiler->owner->current_module_record->is_repl)) {
		type_table_set(compiler->owner->current_module_record->types, struct_name_str, struct_type);
	}

	pop(compiler->owner->current_module_record); // struct_type
	for (int i = 0; i < fieldCount; i++) {
		pop(compiler->owner->current_module_record); // field_type
		pop(compiler->owner->current_module_record); // fieldName
	}
	pop(compiler->owner->current_module_record); // field_types
	pop(compiler->owner->current_module_record); // structObject
	pop(compiler->owner->current_module_record); // struct_name_str
}

void impl_declaration(Compiler *compiler)
{
	consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected struct name after 'impl'.");
	const Token struct_name_token = compiler->parser->previous;
	const ObjectString *struct_name_str = copy_string(compiler->owner, struct_name_token.start,
													  struct_name_token.length);

	ObjectTypeRecord *struct_type = NULL;
	if (!type_table_get(compiler->type_table, struct_name_str, &struct_type) || struct_type->base_type != STRUCT_TYPE) {
		if (struct_type) {
			char got[128];
			type_record_name(struct_type, got, sizeof(got));
			compiler_panicf(compiler->parser, TYPE, "Cannot implement methods for a non-struct type, got '%s'.", got);
		} else {
			compiler_panicf(compiler->parser, TYPE, "Cannot implement methods for a non-struct type '%s'.",
							struct_name_str->chars);
		}
		return;
	}

	named_variable(compiler, struct_name_token, false);
	pop_type_record(compiler);

	consume(compiler, CRUX_TOKEN_LEFT_BRACE, "Expected '{' before impl body.");

	while (!check(compiler, CRUX_TOKEN_RIGHT_BRACE) && !check(compiler, CRUX_TOKEN_EOF)) {
		consume(compiler, CRUX_TOKEN_FN, "Expected 'fn' inside impl block.");
		consume(compiler, CRUX_TOKEN_IDENTIFIER, "Expected method name.");

		const Token method_name_tok = compiler->parser->previous;
		ObjectString *method_name_str = copy_string(compiler->owner, method_name_tok.start, method_name_tok.length);
		push(compiler->owner->current_module_record, OBJECT_VAL(method_name_str));
		const uint16_t method_name_const = make_constant(compiler, OBJECT_VAL(method_name_str));

		// slot 0 is preserved for self
		function(compiler, TYPE_METHOD, struct_type, NULL, -1);

		ObjectTypeRecord *method_type = pop_type_record(compiler);
		push(compiler->owner->current_module_record, OBJECT_VAL(method_type));
		type_table_set(struct_type->as.struct_type.field_types, method_name_str, method_type);

		emit_words(compiler, OP_METHOD, method_name_const);
		pop(compiler->owner->current_module_record); // method_type
		pop(compiler->owner->current_module_record); // method_name_str
	}

	consume(compiler, CRUX_TOKEN_RIGHT_BRACE, "Expected '}' after impl body.");

	emit_word(compiler, OP_POP);
}

void type_declaration(Compiler *compiler, bool is_public)
{
	const uint16_t global = parse_variable(compiler, "Expected type name.");

	const Token type_name_token = compiler->parser->previous;
	ObjectString *type_name_str = copy_string(compiler->owner, type_name_token.start, type_name_token.length);
	push(compiler->owner->current_module_record, OBJECT_VAL(type_name_str));
	consume(compiler, CRUX_TOKEN_EQUAL, "Expected '=' after type name.");

	ObjectTypeRecord *aliased_type = parse_type_record(compiler);
	push(compiler->owner->current_module_record, OBJECT_VAL(aliased_type));
	consume(compiler, CRUX_TOKEN_SEMICOLON, "Expected ';' after type declaration.");

	if (compiler->scope_depth == 0) {
		type_table_set(compiler->type_table, type_name_str, aliased_type);
	} else {
		compiler->locals[compiler->local_count - 1].type = aliased_type;
	}

	emit_constant(compiler, OBJECT_VAL(aliased_type));
	define_variable(compiler, global, is_public);

	if (is_public || (compiler->owner->current_module_record && compiler->owner->current_module_record->is_repl)) {
		type_table_set(compiler->owner->current_module_record->types, type_name_str, aliased_type);
	}
	pop(compiler->owner->current_module_record);
	pop(compiler->owner->current_module_record);
}


void public_declaration(Compiler *compiler)
{
	if (compiler->scope_depth > 0) {
		compiler_panic(compiler->parser, "Cannot declare public members in a local scope.", SYNTAX);
	}
	emit_word(compiler, OP_PUB);
	if (match(compiler, CRUX_TOKEN_FN)) {
		fn_declaration(compiler, true);
	} else if (match(compiler, CRUX_TOKEN_LET)) {
		var_declaration(compiler, true);
	} else if (match(compiler, CRUX_TOKEN_STRUCT)) {
		struct_declaration(compiler, true);
	} else if (match(compiler, CRUX_TOKEN_TYPE)) {
		type_declaration(compiler, true);
	} else if (match(compiler, CRUX_TOKEN_USE)) {
		use_statement(compiler, true);
	} else if (match(compiler, CRUX_TOKEN_NATIVE)) {
		native_declaration(compiler, true);
	} else {
		compiler_panic(compiler->parser, "Expected 'fn', 'let', 'struct', 'type', 'use' or 'native' after 'pub'.", SYNTAX);
	}
}

void declaration(Compiler *compiler)
{
	if (match(compiler, CRUX_TOKEN_LET)) {
		var_declaration(compiler, false);
	} else if (match(compiler, CRUX_TOKEN_FN)) {
		fn_declaration(compiler, false);
	} else if (match(compiler, CRUX_TOKEN_STRUCT)) {
		struct_declaration(compiler, false);
	} else if (match(compiler, CRUX_TOKEN_TYPE)) {
		type_declaration(compiler, false);
	} else if (match(compiler, CRUX_TOKEN_PUB)) {
		public_declaration(compiler);
	} else if (match(compiler, CRUX_TOKEN_IMPL)) {
		impl_declaration(compiler);
	} else if (match(compiler, CRUX_TOKEN_NATIVE)) {
		native_declaration(compiler, false);
	} else {
		statement(compiler);
	}

	if (compiler->parser->panic_mode)
		synchronize(compiler);
}
