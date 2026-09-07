# Slots `include/crux.h:54` `examples/embedding/README.md:116`

Move via numbered slots:

- `crux_ensure_slots(vm,n)` reserve `vm.api_stack` `src/crux.c:112`
- `crux_get_slot_count/type` check
- Setters `crux_set_slot_int/double/bool/string/nil/handle` `crux.h:54`
- Getters `crux_get_slot_int/double/bool/string` + `crux_get_variable(vm,module,name,slot)`
- `crux_call(vm,argCount)` — slot0 callable, 1..argCount args, 0 result `crux.h:54`
- Native `CruxForeignMethodFn` `vm` slots: `0` result, `1..` args (`CRUX_QNAN` `crux.h:54` tagging)

```c
crux_ensure_slots(vm,2);
crux_set_slot_int(vm,1,32);
```

Validate via `crux_get_slot_type(vm,slot)` before `get`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
