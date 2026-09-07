# Core `src/native/core.c` `vm.core_fns` `native_registration.c:258`

Global fns (no `use`):

| Fn | Signature | Return | Notes |
|----|-----------|--------|-------|
| `len` | `(Any)->Int` | `Int` | `len([1,2])` `len("hi")` `len({"a":1})` |
| `error` | `(Any)->Error` | `Error` | `error("msg")` |
| `assert` | `(Bool,String)->Nil` | `Nil` | `assert(x==1,"msg")` `panic.c` |
| `int` | `(Any)->Result[Int]` | `Result[Int]` | `int("5")?` `int(3.14)` |
| `float` | `(Any)->Result[Float]` | `Result[Float]` | `float("3.14")?` |
| `string` | `(Any)->String` | `String` | `string(5)` → `"5"` `to_string` `object.h:588` |
| `table` | `(Any)->Result[Table]` | `Result[Table]` | `table({...})` |
| `array` | `(Any)->Result[Array]` | `Result[Array]` |  |
| `format` | `(String,Table[String,Any])->Nil` | `Nil` | `format("hi {name}",{"name":"a"})` |
| `println` | `(Any)->Nil` | `Nil` | `println("hi")` |
| `iter` | `(Any)->Result[Any]` | `Result[Iterator]` | `iter([1,2])?` |
| `next` | `(Any)->Option` | `Option` | `next(iter)` `Option[T]` |

```crux
var n = int("42")?; // Ok(42)
println(string(n)); // "42"
var it = iter([1,2])?; var v = next(it).unwrap(); // 1
var t = table({"a":1})?;
```

Tests `tests/modules/*.crux` via `len` + `iter/next` `tests/features/for_in.crux:36`.

See [Table](table.md) `hashable`, [Error](error.md).
