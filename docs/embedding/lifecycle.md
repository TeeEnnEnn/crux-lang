# Lifecycle `include/crux.h:38` + `examples/embedding/README.md:38`

```c
CruxConfiguration config; init_crux_configuration(&config);
config.writeFn=host_write; config.errorFn=host_error;
config.bindForeignMethodFn=bind_foreign;
CruxVM* vm=crux_vm_new(&config);
crux_interpret(vm,"main",source);
crux_ensure_slots(vm,2);
crux_get_variable(vm,"main","add_ten",0);
crux_set_slot_int(vm,1,32);
if(crux_call(vm,1)==CRUX_INTERPRET_OK) { int v=crux_get_slot_int(vm,0); }
crux_vm_free(vm);
```

Steps `embedding/README.md:317`: link `crux_lib` (`target_link_libraries(your_app PRIVATE crux_lib m)` `CMakeLists.txt:81`), `#include "crux.h"`, `writeFn/errorFn`, `bindForeignMethodFn`, `vm_new`, `interpret`, `get_variable`, `call`, `release_handle`.

`CRUX_INTERPRET_OK/COMPILE_PANIC/RUNTIME_PANIC/EXIT` `crux.h:44` → `exit 65/70` `cli`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
