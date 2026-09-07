# Build `CMakeLists.txt:1` `tools/README`

Options (`OFF` except `CRUX_TAGGED_OBJECT=ON`):
| Flag | Effect |
|------|--------|
| `CRUX_STACK_SAFETY` | `push/pop` checks `vm.h:125` |
| `CRUX_DEBUG_TRACE_EXECUTION` | dispatch print `vm_run.c` |
| `CRUX_DEBUG_PRINT_CODE` | `disassemble_chunk` `debug.c` |
| `CRUX_DEBUG_LOG_GC`/`STRESS_GC` | `gc_last_*` |
| `CRUX_TAGGED_OBJECT` | 48-bit NaN-box `object.h:144`, fails on 32-bit |
| `CRUX_VERSION` | `dev` → `-DCRUX_VERSION=0.22.0` |

Build types `Debug/ASAN/Release/Perf` `CMakeLists.txt:22`: `Debug/ASAN` `-g3 -O0 -Wall`, `ASAN` `-fsanitize=address,undefined` Unix only, `Release` `-O3 -fno-delete-null-pointer-checks -flto`, `Perf` `-O2 -g`.

```bash
cmake -DCMAKE_BUILD_TYPE=Release -S .. -DCRUX_TAGGED_OBJECT=ON
cmake --build .
```

Sources `GLOB` `src/*.c + native/vm/memory/compiler/type_system/object`, `crux_lib` STATIC (`target_include_directories PRIVATE src/_headers deps/utf8 deps/json`), `crux` CLI (`deps/json/cJSON.c`+`linenoise` Unix), `crux_embedding_example`.

`CMAKE_C_COMPILER gcc` `CMakeLists.txt:3`, Windows `cmake -G "MinGW Makefiles"`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
