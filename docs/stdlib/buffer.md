# Buffer `src/native/buffer.c` `vm.buffer_type:580` `src/_headers/object/object.h:530`

Import and construct a buffer with `use Buffer from "crux:buffer";`.

```crux
use Buffer from "crux:buffer";
var buf = Buffer();
```

## Methods

| Method | Signature | Return |
|--------|-----------|--------|
| `write_byte` | `(Buffer, Int)` | `Result[Nil]` |
| `write_int16_le` / `write_int16_be` | `(Buffer, Int)` | `Result[Nil]` |
| `write_int32_le` / `write_int32_be` | `(Buffer, Int)` | `Result[Nil]` |
| `write_float32_le` / `write_float32_be` | `(Buffer, Float)` | `Result[Nil]` |
| `write_float64_le` / `write_float64_be` | `(Buffer, Float)` | `Result[Nil]` |
| `write_string` | `(Buffer, String)` | `Result[Nil]` |
| `write_buffer` | `(Buffer, Buffer)` | `Result[Nil]` |
| `read_byte` | `(Buffer)` | `Result[Int]` |
| `read_string/read_line/read_all` | `(Buffer[,Int])` | `Result[String]` |
| `read_int16_le` / `read_int16_be` | `(Buffer)` | `Result[Int]` |
| `read_int32_le` / `read_int32_be` | `(Buffer)` | `Result[Int]` |
| `read_float32_le` / `read_float32_be` | `(Buffer)` | `Result[Float]` |
| `read_float64_le` / `read_float64_be` | `(Buffer)` | `Result[Float]` |
| `capacity` | `(Buffer)` | `Float` |
| `is_empty` | `(Buffer)` | `Bool` |
| `clear` | `(Buffer)` | `Nil` |
| `peek_byte` | `(Buffer)` | `Int` |
| `skip_bytes` | `(Buffer, Int)` | `Result[Nil]` |
| `clone` | `(Buffer)` | `Buffer` |
| `compact` | `(Buffer)` | `Nil` |

Capacity `INITIAL_BUFFER_CAPACITY 64` `common.h:23`.

```crux
var b = Buffer();
b.write_string("hi")?;
b.write_int32_le(42)?;
var s = b.read_string(2)?; // "hi"
println(string(b.capacity())); // Float
```

Tests `tests/modules/buffer.crux` covers `write_*`/`read_*` + `clear`/`compact`.

See [Core](core.md) `Result` handling `?`.
