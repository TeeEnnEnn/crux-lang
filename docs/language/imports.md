# Imports `src/compiler/compiler_statements.c:5`, `src/file_handler.c:195`

## Forms

```crux
use name from "crux:math";          // native module pi, sin
use pi, sin from "crux:math";       // multiple
use (a as b) from "crux:math";      // alias
use greet from "./greet.crux";       // relative file
use Stack from "std:collections";    // std pkg pkg.crux
use my_func from "pkg:math";         // installed dep crux_modules/math/pkg.crux
pub use Stack from "./stack.crux";   // re-export
```

- `use ... from "crux:X"` native (`src/native/*`), `std:X` → `CRUX_STDLIB` `std/X/pkg.crux` else `X.crux`, `pkg:X` → upward `crux_modules/X/pkg.crux` bounded by `crux.json` (`is_valid_package_name` `file_handler.c:162`), file → `combine_paths`.
- `pub use` only top-level, re-exports via `pkg.crux`.

## Circular

`ImportStack` `vm.h:125` `is_in_import_stack` `vm_helpers.c:774` detects cycles → `IMPORT` panic.

See [Project Layout](../getting-started/project-layout.md).

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
