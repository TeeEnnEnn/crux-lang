# GC `src/native/gc.c` module `gc` `native_registration.c:420` `vm.h:125`

Fns `use ... from "crux:gc"`:

| Fn | Signature | Return |
|----|-----------|--------|
| `off/on` | `()` | `Nil` |
| `collect` | `()` | `Nil` — `crux_collect_garbage` `crux.h:91` |
| `heap_used/heap_capacity` | `()` | `Float` — `bytes_allocated/next_gc` |
| `set_heap_growth` | `(Numeric)` | `Result[Nil]` — `heapGrowthPercent` `src/crux.c:112` |
| `set_min_heap/set_min_growth` | `(Numeric)` | `Result[Nil]` |
| `is_on` | `()` | `Bool` |
| `stats` | `()` | `Table` — `gc_last_*` `vm.h:125` |

```crux
use collect, heap_used, set_heap_growth from "crux:gc";
collect();
println(string(heap_used()));
set_heap_growth(50)?;
```

Tuning also via `CruxConfiguration` `initialHeapSize 5MB` `src/crux.c:112`. Tests `tests/modules/gc.crux` + `tests/benchmarks/gc_harness.py`.

See [Contributor GC](../contributor/gc.md).

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
