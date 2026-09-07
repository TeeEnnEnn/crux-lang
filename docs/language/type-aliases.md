# Type Aliases `src/compiler/compiler_declarations.c:228` `type_table.c`

```crux
type MyInt = Int;
type StringOrInt = String|Int;
pub type Vec2 = Vector[2];
type MyShape = shape {x:Int,y:Int};
var x: MyInt = 5;
var y: StringOrInt = "hi"; y = 5;
```

- `type Name = RealType;` + `pub type` top-level, stored `ObjectTypeTable` `object.h:588` `new_type_table` `common.h:32` `INITIAL_TYPE_TABLE_SIZE 16`.
- Alias transparent `types_equal` unwraps `object.h:362` `type_table.c`.
- No generics on aliases yet (`type My[T] = Array[T]` unsupported `parse_type_record` `compiler_core.c:23`).
- `pub type` exported for `use`? `pub use` re-exports `pkg.crux` `file_handler.c:195`.
- Example `tests/compiler/basic_types.crux:24` `Array[Any]` vs `type MyArray = Array[Int]` would be `Int|Float` etc not yet.

See [Types](types.md) `TypeMask` `value.h:47`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
