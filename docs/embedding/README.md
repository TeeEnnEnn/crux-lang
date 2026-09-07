# Embedding Index `include/crux.h:1` `examples/embedding/README.md:1` 333 lines

Crux as embedded scripting (Wren-inspired) `crux.h:38`:

- [Lifecycle](lifecycle.md) — 11-step `init_configuration` → `vm_new` → `interpret` → `get_variable` → `call` → `free` (`examples/embedding/main.c:6`)
- [Slots](slots.md) — `crux_ensure_slots`, `set/get_slot_*`, `crux_get_slot_type` `crux.h:54`
- [Handles](handles.md) — `CruxHandle` keep-alive `crux_get_slot_handle` `crux.h:38`
- [Foreign Functions](foreign-functions.md) — `native fn` + `bindForeignMethodFn` `crux.h:77` signature matching
- [Modules](modules.md) — `resolveModuleFn`/`loadModuleFn` `CruxLoadModuleResult` `crux.h:77` (`src/file_handler.c:195`)
- [GC Tuning](gc-tuning.md) — `initialHeapSize` `gc` module `native/gc.c`
- [Examples](examples.md) — link to [`examples/embedding/main.c`](../../examples/embedding/main.c) + raylib sketch `embedding/README.md:287`

Public API `include/crux.h:1` `CRUX_VERSION_STRING "0.22.0"` `CruxInterpretResult` `CruxErrorType`. Build `target_link_libraries(your_app PRIVATE crux_lib m)` `CMakeLists.txt:81`.

See [Native API](../reference/native-api.md) table.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
