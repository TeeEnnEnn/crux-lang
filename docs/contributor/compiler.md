# Compiler `src/compiler/*.c` `src/README.md:3` `src/_headers/compiler/*.h` 669+303+2156+342+268+419+598+530 lines split `1fc77c6`

Pipeline `scanner.c` → `pre_compiler.c` → `compiler_core.c` + helpers:

- `scanner.c:167` `scan_token` `Scanner {start,current,line}` `scanner.h:10` 106 `CruxTokenType` (`**`, `::`, `$[`), `CRUX_TOKEN_STRING` both `'`/`"` escapes.
- `pre_compiler.c:441` two-pass `pub struct`/`pub fn` forward scan `pre_advance` `pre_skip_block` `pre_skip_parens` `pre_skip_type` for `shape {}`.
- `compiler_core.c:23` `parse_type_record` handles `shape {field:Type}`, `Array[T]`, `Table[K,V]` (`is_valid_table_key_type` `HASHABLE`), `Vector[dim]` `-1` wildcard, `Matrix`, `Tuple[T,…]`, `Result[Ok]`, `Option[Some]`, `(A)->Ret`, `A|B` union, identifier `TypeAlias`/`Struct`.
- `compiler_helpers.c:54` `check/match/consume`, `type_token_type_to_mask` `CRUX_TOKEN_INT_TYPE`→`INT_TYPE`, `lookup_stdlib_method` `vm.array_type`.
- `compiler_declarations.c:58` `var` (annotated `var x:Int=expr` else `T_NIL/T_ANY`), `struct` `pub struct S {field:Type}`, `impl` `fn` vs `static fn`, `type Alias = RealType`, `pub` top-level.
- `compiler_statements.c:30` `if/while/for` `for var x in iterable`, `use` `pub use/as/from` `compile_module_statically` `ImportStack` `vm.h:125`, `panic`, `break/continue` `LOOP_WHILE/FOR`.
- `compiler_functions.c:12` `fn name(params:Type)->Ret`, closures `upvalue` `Function` `Closure` `object.h:263`, `recursive_name`.
- `compiler_match.c:1` `match` `MatchHandlerStack` exhaustiveness `MatchExhaustiveness` `has_ok/has_err/default`.
- `compiler_expressions.c:412` Pratt `parse_precedence` `Precedence` `number/grouping/binary/unary` `OP_ANON_FUNCTION`.

Out `Chunk` `object.h:252` `ObjectFunction {arity,Chunk,module_record}` + `CallFrame`. Debug `DEBUG_PRINT_CODE` `disassemble_chunk` `compiler_core.c:541` `debug.c`.

See [Language](../language/overview.md), [Architecture](architecture.md), [VM](vm.md).

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
