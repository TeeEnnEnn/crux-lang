# FS `src/native/fs.c` module `fs` `native_registration.c:711`

Fns `use ... from "crux:fs"` (all `Result` unless `Bool`):

| Fn | Signature | Return |
|----|-----------|--------|
| `open` | `(String path,String mode)` | `Result[File]` `mode` `r/w/a` |
| `remove/remove_dir` | `(String)` | `Result[Nil]` |
| `size` | `(String)` | `Result[Int]` |
| `copy_file` | `(String src,String dst)` | `Result[Nil]` |
| `mkdir` | `(String)` | `Result[Nil]` |
| `read_file/write_file/append_file` | `(String[,String])` | `Result[String]/Result[Nil]` |
| `exists/is_file/is_dir` | `(String)` | `Bool` |

```crux
use open, read_file, write_file, exists from "crux:fs";
var f = open("hi.txt","w")?;
f.writeln("hello")?;
f.close()?;
var content = read_file("hi.txt")?; // "hello\n"
println(string(exists("hi.txt"))); // true
```

`ObjectFile` `object.h:444` `FILE*` `is_open`. Tests `tests/modules/fs.crux`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
