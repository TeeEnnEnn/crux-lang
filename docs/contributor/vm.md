# VM `src/vm/vm_run.c` `src/vm/vm_helpers.c` `src/_headers/chunk.h:6` `src/_headers/vm.h:125` `src/_headers/object/object.h:500`

## State `vm.h:125` `ObjectModuleRecord`

- `CallFrame` `closure/ip/slots` — `ip` points into `Chunk.code`, `slots` window into `module_record->stack`.
- `ObjectModuleRecord` `stack/stack_top/stack_limit/frames` `global_names/publics/types` `globals` `ObjectTypeTable types` `open_upvalues` `object.h:500` `STATE_LOADING/LOADED/ERROR/EXECUTED` `is_repl/is_main` `frame_count/capacity` `FRAMES_MAX=128` `common.h:16`.
- `NativeModules` `vm.native_modules` `name`+`Table names` `NATIVE_MODULES_CAPACITY 16` `common.h:22`.
- `StructInstanceStack` `vm.struct_instance_stack` `STRUCT_INSTANCE_DEPTH 16`.
- `ImportStack` `paths/count/capacity` `vm_helpers.c:774` `is_in_import_stack` cycle.

## Dispatch `vm_run.c` ~130 `OpCode` `chunk.h:6`

Groups:
- Constant: `OP_CONSTANT` `OP_NIL` `OP_TRUE` `OP_FALSE`
- Arithmetic: `OP_ADD`/`SUBTRACT`/`MULTIPLY`/`DIVIDE`/`INT_DIVIDE`/`MODULUS`/`POWER`/`NEGATE`/`LEFT_SHIFT`/`RIGHT_SHIFT` + compound `OP_SET_GLOBAL_*` `src/compiler/compiler_expressions.c`.
- Logic: `OP_EQUAL`/`NOT_EQUAL`/`GREATER`/`LESS`/`LESS_EQUAL`/`GREATER_EQUAL`/`NOT`/`AND`/`OR`
- Local/Global: `OP_DEFINE_GLOBAL` `OP_GET_GLOBAL` `OP_SET_GLOBAL` (+ `SLASH/STAR/PLUS/MINUS`/`INT_DIVIDE/MODULUS`), `OP_GET_LOCAL`/`SET_LOCAL`, `OP_GET_UPVALUE`/`SET_UPVALUE` `OP_CLOSE_UPVALUE`
- Control: `OP_JUMP` `OP_JUMP_IF_FALSE` `OP_LOOP` `OP_RETURN` `OP_NIL_RETURN` `OP_POP`
- Call: `OP_CALL` `OP_CLOSURE` `OP_INVOKE` `OP_STATIC_INVOKE` `OP_ANON_FUNCTION` `compiler_functions.c`
- Property: `OP_GET_PROPERTY` `OP_SET_PROPERTY` `OP_STRUCT_INSTANCE` `src/compiler/compiler_declarations.c`
- Collection: `OP_ARRAY` `OP_TABLE` `OP_TUPLE` `OP_RANGE` `OP_GET_COLLECTION` `OP_SET_COLLECTION` (`IS_CRUX_HASHABLE` `vm_run.c:689`) `OP_GET_SLICE` `range_indices_in_bounds`
- Match: `OP_MATCH` `OP_MATCH_JUMP` `OP_MATCH_END` `OP_RESULT_MATCH_OK/ERR` `OP_RESULT_BIND` `OP_GIVE` `OP_PUB` `compiler_match.c:1` `MatchHandlerStack` `vm.h:125`.

Method dispatch via `vm.array_type` etc `native_registration.c:313` `init_type_method_table`.

## Debug

`DEBUG_TRACE_EXECUTION` prints `ip`/`stack`, `DEBUG_PRINT_CODE` `debug.c` `disassemble_chunk` `compiler_core.c:541`, `STACK_SAFETY` `push/pop` `STACK_OVERFLOW` `vm.h:125`.

See [Architecture](architecture.md), [Opcodes](../reference/opcodes.md), [Chunk](../reference/opcodes.md).
