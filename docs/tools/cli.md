# CLI `src/cli/README.md:1` `src/cli/main.c:11` `cli_commands.c` `cli_utils.c`

## Commands `cli/main.c:11` `print_usage`

- `crux path.crux` — run script `file_handler.c:195` resolve `std:`/`pkg:`.
- `crux` — REPL `linenoise` `deps/linenoise/linenoise.c` `~/.local/share/crux/repl_history` Unix (no linenoise Windows `deps/linenoise` absent `CMakeLists.txt:105`).
- `crux init [name]` — `cli_commands.c:180` writes `crux.json:43` `{"name":"my_project","version":"0.1.0","main":"main.crux"}` + `main.crux` + `pkg.crux` `pub use`.
- `crux install` — `cJSON` `deps/json/cJSON.c` parse `crux.json` `dependencies` `math: https://github.com/user/math.git` → `git clone` `crux_modules/<name>/` recursive `process_dependencies` `cli_commands.c:94` `src/cli/cli_utils.c`.
- `crux run` — run the `main` path from `crux.json` in the current directory, falling back to `main.crux`.
- `crux run path.crux` — run a specific script. The argument is not a project directory; use `cd project && crux run` for a project.
- `crux -V/--version` `crux.h:23` `CRUX_VERSION_STRING "0.22.0"` `cli/main.c:52`.

## Exit Codes `cli_commands.c:151`

`0` success, `1` VM initialization or project creation failure, `2` unreadable script,
`64` command usage, `65` compile error, and `70` runtime error. `sys.exit(code)` returns
the requested process exit code.

```text
my_project/
├── crux.json
├── main.crux
├── pkg.crux
└── crux_modules/
```

See [Project Layout](../getting-started/project-layout.md), [Build](build.md).

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
