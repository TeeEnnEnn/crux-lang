# IO `src/native/io.c` module `io` `native_registration.c:656`

Fns `use ... from "crux:io"`:

| Fn | Signature | Return |
|----|-----------|--------|
| `print` | `(Any)` | `Nil` |
| `print_to` | `(String, Any)` | `Result[Nil]` |
| `println_to` | `(String, Any)` | `Result[Nil]` |
| `scan/scanln` | `()` | `Result[String]` |
| `nscan` | `(Int)` | `Result[String]` |
| `scan_from/scanln_from` | `(String)` | `Result[String]` |
| `nscan_from` | `(String, Int)` | `Result[String]` |

All `scan*` use `src/_headers/vm.h:125` `writeFn/errorFn` `CruxConfiguration`.

```crux
use scan, print from "crux:io";
print("hi");
var line = scan()?; // Result via file_handler stdin
```

Also core `println(Any)->Nil` `native_registration.c:258`.

Tests `tests/modules/io.crux` + `src/cli/cli_commands.c` REPL.

See [FS](fs.md) for file IO.
