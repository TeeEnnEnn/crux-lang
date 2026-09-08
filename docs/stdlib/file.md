# File `src/native/fs.c` `ObjectFile` `object.h:444` `vm.file_type:381`

Methods `File.` `native_registration.c:381`:

| Method | Signature | Return |
|--------|-----------|--------|
| `close/flush` | `(File)` | `Result[Nil]` |
| `read` | `(File,Int)` | `Result[String]` |
| `readln/read_all` | `(File)` | `Result[String]` |
| `read_lines` | `(File)` | `Result[Array[String]]` |
| `write/writeln` | `(File,String)` | `Result[Nil]` |
| `seek` | `(File,Int,String)` | `Result[Nil]` — `whence` |
| `tell` | `(File)` | `Result[Int]` |
| `is_open` | `(File)` | `Bool` |

Constructor via `fs` `open(path,mode)->Result[File]` `native_registration.c:711`.

```crux
use open from "crux:fs";
var f = open("out.txt","w")?;
f.write("hi")?;
f.writeln(" there")?;
f.close()?;
var g = open("out.txt","r")?;
println(g.read_all()?);
```

Tests `tests/modules/fs.crux` `file` section.

See [FS](fs.md) module.
