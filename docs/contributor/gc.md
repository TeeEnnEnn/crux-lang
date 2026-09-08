# GC `src/memory/garbage_collector.c` `slab_allocator.c` `alloc.c` `vm.h:125` `common.h:28`

## Heap `CruxConfiguration` `src/crux.c:112`

- `initialHeapSize 5MB` `minHeapSize 1MB` `heapGrowthPercent 50` → `vm.initialHeapSize/minHeapSize/heapGrowthPercent` `vm.h:125`.
- `bytes_allocated` `next_gc` `heap_growth_factor 2.0` `INIT_GC_HEAP_GROW_FACTOR 2` `MIN_GC_HEAP_SIZE 1MB` `MIN_GC_GROWTH_DELTA 256KB` `common.h:28`, `TABLE_MAX_LOAD 0.65`.

## Slabs `slab_allocator.c` `alloc.c:12`

- `slab_24/32/48/64` `SLAB_CAPACITY 4096` `common.h:31` — objects `≤24/32/48/64` bytes via `allocate_from_slab`, else `internal_reallocate` → `vm.config.reallocateFn` host hook `vm.h:125` (`CruxReallocateFn` `crux.h:54`) else `malloc`/`realloc`.
- `allocate_object_with_gc` `alloc.c:12` bumps `bytes_allocated`, `if (bytes_allocated > next_gc) collect_garbage(vm)` else `DEBUG_STRESS_GC` every alloc.

## Tri-color `garbage_collector.c`

- `gray_stack` `gray_capacity/count` `vm.h:125`, `object.h:144` `is_marked/is_immortal` `CRUX_TAGGED_OBJECT` 48-bit `tagged_next` (`type 5-bit` `0x1F`, marked `53`, immortal `54`).
- `immortal` `object_set_immortal` `object.h:216` for natives `native_registration.c:150` `callable` + `arg_types/return_type`.
- Mark roots: `vm.objects` list `vm.handles` `CruxHandle` `crux.c:112`, `module_record->stack`/`globals`/`open_upvalues`.
- Stats `gc_collections` `gc_total_ns` `gc_last_*` (`gc_last_total_ns`, `gc_sweep_slots_scanned`, `gc_last_live_objects`) `vm.h:125`.

## API Module `gc` `src/native/gc.c` `native_registration.c:420`

`off/on/collect/heap_used/heap_capacity/set_heap_growth/set_min_heap/set_min_growth/is_on/stats()->Table` `docs/stdlib/gc.md`.

Flags `CRUX_DEBUG_LOG_GC` logs `gc_last_*`, `CRUX_DEBUG_STRESS_GC` `CMakeLists.txt:10`.

See [Stdlib GC](../stdlib/gc.md) + [Build](../tools/build.md).
