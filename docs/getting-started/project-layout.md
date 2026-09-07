# Project Layout

## Manifest `crux.json` (`src/cli/cli_commands.c:180`)

```json
{
  "name": "my_project",
  "version": "0.1.0",
  "main": "main.crux",
  "dependencies": {
    "math": "https://github.com/user/crux-math.git"
  }
}
```
- `name` — package name `[a-zA-Z0-9_-]` (`file_handler.c:162` `is_valid_package_name`).
- `main` — entry for `crux run` (default `main.crux` `src/cli/main.c:67`).
- `dependencies` — git URLs cloned into `crux_modules/<name>/`.

Created by `crux init [name]` (`src/cli/cli_commands.c:198`): writes `crux.json`, `main.crux`, `pkg.crux`.

## Package entry `pkg.crux`

Re-export for consumers via `pkg:`:
```crux
// pkg.crux
pub use Stack, Set from "./stack.crux";
pub use Stack from "./set.crux"; // actually Set from "./set.crux"
```

Std example `std/collections/pkg.crux:1`:
```crux
pub use Stack from "./stack.crux";
pub use Set from "./set.crux";
```
`std/collections/crux.json:4` `main: pkg.crux` (was `main.crux`).

Consumed as:
```crux
use Stack from "std:collections";
use my_func from "pkg:math";
```

## Resolution (`src/file_handler.c:195`)

1. `std:X` → `CRUX_STDLIB` env or `<exe-dir>/stdlib` (`get_crux_dir()+"/stdlib"`) + `X/pkg.crux` else `X.crux`.
2. `pkg:X` → upward traversal from `base_path` dir, looking for `crux_modules/X/pkg.crux`, stopping at `crux.json` boundary (project root) — prevents escaping.
3. Relative `use " ./a.crux"` → `combine_paths(base, relative)` + `realpath`.

## `crux_modules/`

```text
my_project/
├── crux.json
├── main.crux
├── pkg.crux
└── crux_modules/
    └── math/
        ├── crux.json
        ├── pkg.crux
        └── math.crux
```

`crux install` (`src/cli/cli_commands.c:94`) — `cJSON` parses manifest, `git clone` each dep (recursive `process_dependencies`).

## Std dispatch

`std/` in repo: `collections/` `statistics/` `_std_test/`. Installed via `release.yml:104` `cp -r std/* dist/stdlib/` → `crux-stdlib.tar.gz` (`tar -czf -C dist stdlib`). Runtime `CRUX_STDLIB=std ./build/crux std/_std_test/collections_test.crux`.

See also: [CLI](../tools/cli.md) `crux install`/`run`, [Imports](../language/imports.md).
