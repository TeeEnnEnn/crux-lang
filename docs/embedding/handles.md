# Handles `include/crux.h:38` `src/crux.c:112` `vm.h:125` `examples/embedding/README.md:238`

## API `crux.h:54`

```c
CruxHandle* h = crux_get_slot_handle(vm,0);
crux_set_slot_handle(vm,0,h);
crux_release_handle(vm,h);
```

`CruxHandle` `struct {CruxValue value; CruxHandle *prev,*next;}` `vm.h:125` linked list `vm.handles` GC roots (`mark_object_table` `garbage_collector.c`). Protects `CruxValue` `uint64_t` NaN-box `CRUX_QNAN` `crux.h:54` beyond slot window.

## When Use

- Long-lived Crux values in host structs (store handle, not `CruxValue`).
- Callbacks `crux_get_variable` → handle → later `crux_call` without re-lookup.
- Not needed for immediate `crux_call` where slots suffice (`slot0 callable, 1..args`).

## Pitfalls

- `crux_ensure_slots(vm,n)` before `get_variable` `main.c:77` else `CRUX_TYPE_UNKNOWN`.
- Handles are immortal until `release_handle`, else leak `vm.objects` → `gray_stack` overflow.
- `CruxValue` raw copy unsafe across GC; handle ensures `is_marked` `object.h:192`.

Example `examples/embedding/main.c:77` `crux_get_slot_handle` then `crux_release_handle` on shutdown.

See [Slots](slots.md), [GC Tuning](gc-tuning.md).
