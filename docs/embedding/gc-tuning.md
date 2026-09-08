# GC Tuning `include/crux.h:91` `vm.h:125` `src/native/gc.c` `common.h:28`

## Heap Config `CruxConfiguration` `src/crux.c:112`

```c
CruxConfiguration config; init_crux_configuration(&config);
config.initialHeapSize = 5*1024*1024; // 5MB
config.minHeapSize = 1*1024*1024;    // 1MB
config.heapGrowthPercent = 50;      // 50%
```

Fields `vm.h:125`: `initialHeapSize` `minHeapSize` `heapGrowthPercent` → `vm.heap_growth_factor` `2.0` `MIN_GC_HEAP_SIZE` `MIN_GC_GROWTH_DELTA` `common.h:28`.

## API Module `gc` `src/native/gc.c:420`

`use ... from "crux:gc"`:

| Fn | Effect |
|----|--------|
| `off/on` | pause/resume `vm.gc_status PAUSED/RUNNING` |
| `collect` | `crux_collect_garbage(vm)` `crux.h:91` |
| `heap_used/heap_capacity` | `vm.bytes_allocated/next_gc` |
| `set_heap_growth(Numeric)` | `heapGrowthPercent` `Result[Nil]` |
| `set_min_heap/set_min_growth` | `Result[Nil]` |
| `is_on` | `Bool` |
| `stats()->Table` | `gc_last_total_ns`, `gc_sweep_slots_scanned`, `gc_last_live_objects` `vm.h:125` |

## Allocator Hook

`reallocateFn` `CruxReallocateFn(void*memory,size_t newSize,void*userData)` `alloc.c:12` `internal_reallocate` else `malloc`, `userData` `config.userData`, longjmp on OOM `src/memory/alloc.c:12`.

Flags `CRUX_TAGGED_OBJECT` `object.h:144` 48-bit.

See [Embedding Handles](handles.md).
