# Modules `include/crux.h:77` `src/file_handler.c:195` `vm.h:125`

## Host Callbacks `CruxConfiguration` `crux.h:91`

- `CruxResolveModuleFn resolveModuleFn(CruxVM* vm,const char* importer,const char* name)->const char*` — rewrite import name (e.g. `"crux:math"` → `"math"`).
- `CruxLoadModuleFn loadModuleFn(CruxVM* vm,const char* name)->CruxLoadModuleResult{ const char* source; CruxLoadModuleCompleteFn onComplete; void* userData }` — host provides source text, `onComplete(vm,name,result)` called after compile, `userData` for async.
- `CruxConfiguration scriptPath` (`src/cli/main.c:52` fallback `main.crux`) + `userData` passed through callbacks.

## Resolution `file_handler.c:195`

1. `std:X` → `CRUX_STDLIB` env or `<exe-dir>/stdlib` (`get_crux_dir()+"/stdlib"`) + `X/pkg.crux` else `X.crux`.
2. `pkg:X` → upward `crux_modules/X/pkg.crux` bounded by `crux.json` (`is_valid_package_name` `file_handler.c:162`).
3. File `use "..."` → `combine_paths(base,relative)` + `realpath`.

## State `vm.h:125`

`ImportStack paths/count/capacity` `is_in_import_stack` `vm_helpers.c:774` detects cycles → `IMPORT` panic `panic.c:69` `IMPORT_EXTENT`.

Example `examples/embedding/main.c:6` `crux_interpret(vm,"example",source)` module name for `crux_get_variable`.

See [Lifecycle](lifecycle.md), [Slots](slots.md).

## Notes

- Cross-linked from [docs/README.md](../README.md) and [`src/README.md`](../../src/README.md).
