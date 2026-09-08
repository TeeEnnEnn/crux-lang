# Sys `src/native/sys.c` module `sys` `native_registration.c:697`

Fns `use ... from "crux:sys"`:

| Fn | Signature | Return |
|----|-----------|--------|
| `args` | `()` | `Result[Array[String]]` — `vm.args` `vm.h:125` |
| `get_env` | `(String)` | `Result[String]` |
| `platform` | `()` | `String` — `linux/windows/macos` |
| `arch` | `()` | `String` — `amd64/arm64` |
| `pid` | `()` | `Int` |
| `exit` | `(Int)` | `Never` — `RUNTIME_EXIT_CODE 70` `common.h:19` |

```crux
use args, platform, exit from "crux:sys";
println(platform()); // "linux"
var a = args()?; // ["crux","hello.crux"]
if len(a) > 1 { exit(0); }
```

Tests `tests/modules/sys.crux`.

See [Build](../tools/build.md) `CRUX_VERSION`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
