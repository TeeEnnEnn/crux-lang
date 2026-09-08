# Stdlib Index

Crux has four distinct kinds of reusable API:

1. **Core globals**, such as `len` and `println`, require no import.
2. **Built-in type methods**, such as `Array.push`, are available on values of that type.
3. **Native modules** are compiled into Crux and imported with `crux:`.
4. **Source packages** ship as Crux code and are imported with `std:`.

Fallible operations return `Result[T]`; handle them with `match`, `unwrap_or()`, or explicit
status checks. Both `unwrap()` and `?` produce the success value and panic on `Err`.

| Module | File `src/native/*.c` | Crux `use` |
|--------|-----------------------|------------|
| Core | `core.c` | `len/error/assert/int/float/string/table/array/format/println/iter/next` (global) |
| String | `string.c` | methods `src/native/string.c` |
| Array | `array.c` | `Array[T]` methods |
| Table | `tables.c` | `Table[K,V]` methods |
| Result | `result.c` | `Result[T]` methods |
| Option | `option.c` | `Option[T]` methods |
| Error | `error.c` | `Error` methods |
| File | — | `File` via `fs` |
| Random | `random.c` | `use Random from "crux:random"` |
| GC | `gc.c` | `use ... from "crux:gc"` |
| Vector | `vectors.c` + `vector_helpers.c` | `Vector[3]` |
| Complex | `complex.c` | `Complex` |
| Matrix | `matrix.c` | `Matrix[2,3]` |
| Range | `range.c` | `Range` |
| Tuple | `tuple.c` | `Tuple` |
| Buffer | `buffer.c` | `Buffer` |
| Math | `math.c` | `use ... from "crux:math"` |
| IO | `io.c` | `use ... from "crux:io"` |
| Time | `time.c` | `use ... from "crux:time"` |
| Sys | `sys.c` | `use ... from "crux:sys"` |
| FS | `fs.c` | `use ... from "crux:fs"` |
| Collections | `std/collections/*.crux` | `use Stack,Set from "std:collections"` |
| Statistics | `std/statistics/*.crux` | `use mean,median from "std:statistics"` |

Tests: `tests/modules/*.crux` (16), `std/_std_test/*.crux`.

The callable inventory comes from `initialize_std_lib` in
`src/native/native_registration.c`. `scripts/check_docs.py` verifies that every registered
name and every public source-package export is represented in these pages.

See [Native Contributor](../../src/native/README.md).
