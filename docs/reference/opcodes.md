# Opcodes

`OpCode` in `src/_headers/chunk.h` is the authoritative and exhaustive opcode list.
Opcodes are an internal implementation detail and may change without compatibility
support. The VM dispatch implementation is in `src/vm/vm_run.c`.

| Group | Opcodes |
|-------|---------|
| Constant | `OP_CONSTANT` `OP_NIL` `OP_TRUE` `OP_FALSE` |
| Arithmetic | `OP_ADD` `OP_SUBTRACT` `OP_MULTIPLY` `OP_DIVIDE` `OP_INT_DIVIDE` `OP_MODULUS` `OP_POWER` `OP_NEGATE` `OP_LEFT_SHIFT` `OP_RIGHT_SHIFT` |
| Logic | `OP_EQUAL` `OP_NOT_EQUAL` `OP_GREATER` `OP_LESS` `OP_LESS_EQUAL` `OP_GREATER_EQUAL` `OP_NOT` `OP_BITWISE_AND` `OP_BITWISE_XOR` `OP_BITWISE_OR` `OP_BITWISE_NOT` |
| Global/Local | `OP_DEFINE_GLOBAL` `OP_GET_GLOBAL` `OP_SET_GLOBAL` (+ `OP_SET_GLOBAL_*` compound `+=` etc) `OP_GET_LOCAL` `OP_SET_LOCAL` (`OP_SET_LOCAL_*`), `OP_GET_UPVALUE`/`SET_UPVALUE` `OP_CLOSE_UPVALUE` |
| Control | `OP_JUMP` `OP_JUMP_IF_FALSE` `OP_LOOP` `OP_RETURN` `OP_NIL_RETURN` `OP_POP` `OP_POP_N` `OP_PANIC` |
| Call | `OP_CALL` `OP_CLOSURE` `OP_INVOKE` `OP_STATIC_INVOKE` `OP_ANON_FUNCTION` `OP_METHOD` `OP_STATIC_METHOD` |
| Property | `OP_GET_PROPERTY` `OP_SET_PROPERTY` `OP_GET_PROPERTY_INDEX` `OP_SET_PROPERTY_INDEX` and compound variants |
| Collection | `OP_ARRAY` `OP_TABLE` `OP_TUPLE` `OP_RANGE` `OP_GET_COLLECTION` `OP_SET_COLLECTION` `OP_GET_SLICE` |
| Match | `OP_MATCH` `OP_MATCH_JUMP` `OP_MATCH_END` `OP_RESULT_MATCH_OK` `OP_RESULT_MATCH_ERR` `OP_OPTION_MATCH_SOME` `OP_OPTION_MATCH_NONE` `OP_TYPE_MATCH` `OP_RESULT_BIND` `OP_GIVE` |
| Result/Option | `OP_OK` `OP_ERR` `OP_SOME` `OP_NONE` `OP_UNWRAP` |
| Types/structs | `OP_TYPEOF` `OP_TYPE_COERCE` `OP_STRUCT` `OP_STRUCT_INSTANCE_START` `OP_STRUCT_NAMED_FIELD` `OP_STRUCT_INSTANCE_END` |
| Modules | `OP_USE_MODULE` `OP_FINISH_USE` `OP_FINISH_PUB_USE` `OP_BIND_NATIVE` `OP_DEFINE_PUB_GLOBAL` |
| Iteration | `OP_IN` `OP_ITER_INIT` `OP_ITER_NEXT` |
| Native calls | `OP_INVOKE_STDLIB` `OP_INVOKE_STDLIB_UNWRAP` |
| Specialized arithmetic | `OP_ADD_INT`, `OP_ADD_NUM`, and vector, matrix, and complex specializations |

The compiler emits specialized arithmetic opcodes when static types make the operation
known. Generic opcodes retain dynamic checks. Compound assignment has local, global,
upvalue, property, and indexed-property variants.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
