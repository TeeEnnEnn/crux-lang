# Debugging `CMakeLists.txt:26` `vm.h:125` `debug.c`

Build `cmake -DCMAKE_BUILD_TYPE=Debug -DCRUX_DEBUG_TRACE_EXECUTION=ON -DCRUX_DEBUG_PRINT_CODE=ON -DCRUX_DEBUG_LOG_GC=ON -DCRUX_DEBUG_STRESS_GC=ON -DCRUX_STACK_SAFETY=ON`.

| Flag | Effect |
|------|--------|
| `CRUX_STACK_SAFETY` | `push/pop` `STACK_OVERFLOW` `STACK_UNDERFLOW` checks `vm.h:125` |
| `CRUX_DEBUG_TRACE_EXECUTION` | `vm_run.c` prints `ip`/`stack` each dispatch |
| `CRUX_DEBUG_PRINT_CODE` | `disassemble_chunk` `debug.c` after `compiler_core.c:541` |
| `CRUX_DEBUG_LOG_GC` | `gc` `gc_last_*` `vm.h:125` `marked/sweep` |
| `CRUX_DEBUG_STRESS_GC` | `collect_garbage` on every `alloc.c:12` |
| `CRUX_TAGGED_OBJECT` | NaN-box `object.h:144` |

Other `ASAN` `cmake -DCMAKE_BUILD_TYPE=ASAN` Unix only `-fsanitize=address,undefined` `CMakeLists.txt:22`.

`panic.c:69` `ErrorType` 23 `SYNTAX`…`IO` via `vm_error` `CRUX_ERROR_STACK_TRACE` `include/crux.h:54`.

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DCRUX_DEBUG_TRACE_EXECUTION=ON -S . -B build
cmake --build build && ./build/crux test.crux
```

## Notes

- Cross-linked from [docs/README.md](../README.md) and [`src/README.md`](../../src/README.md).
