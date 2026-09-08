# Installation

## Quick install (release binary + stdlib)

**Unix (Linux/macOS):**
```bash
curl -fsSL https://raw.githubusercontent.com/TheophilusNenhanga/crux-lang/main/scripts/install/install.sh | bash
# specific version
curl -fsSL https://raw.githubusercontent.com/TheophilusNenhanga/crux-lang/main/scripts/install/install.sh | bash -s -- v0.22.0
```

**Windows (PowerShell):**
```powershell
iwr https://raw.githubusercontent.com/TheophilusNenhanga/crux-lang/main/scripts/install/install.ps1 | iex
```

Environment overrides (see `scripts/install/install.sh:8`):
- `CRUX_INSTALL_DIR` default `~/.local/bin` — where `crux` (+ `crux.exe`) is placed. Add to `PATH`.
- `CRUX_STDLIB_DIR` default `~/.local/share/crux/stdlib` — `std/collections` + `statistics` are unpacked here. `src/file_handler.c:195` resolves `std:` via `CRUX_STDLIB` or `<exe-dir>/stdlib`.
- `CRUX_BASE_URL` — override download base for local testing, e.g. `CRUX_BASE_URL=http://127.0.0.1:8766 bash scripts/install/install.sh v0.22.0` with `python3 -m http.server`.

What the script fetches (`release.yml:101`):
- `crux-linux-amd64`, `crux-windows-amd64.exe`, `crux-macos-amd64`, `crux-macos-arm64`
- `crux-stdlib.tar.gz` (`tar -czf -C dist stdlib`) / `crux-stdlib.zip` (`zip -r dist/stdlib`)

Manual download: pick asset from [Releases](https://github.com/TheophilusNenhanga/crux-lang/releases) `Manual Downloads` section.

## Build from source

Requirements: `gcc`, `cmake >=3.28` (`CMakeLists.txt:1`), `make` or `MinGW Makefiles` on Windows.

**Linux (Release):**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -S ..
cmake --build .
./build/crux --version # Crux 0.22.0
```

**Windows (MinGW):**
```powershell
cmake -G "MinGW Makefiles" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\crux.exe --version
```

**Debug / ASAN / Perf:**
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -S .. -DCRUX_DEBUG_TRACE_EXECUTION=ON -DCRUX_TAGGED_OBJECT=ON
cmake -DCMAKE_BUILD_TYPE=ASAN -S .. # only UNIX CMakeLists.txt:22
cmake -DCMAKE_BUILD_TYPE=Perf -S .. # -O2 -g -fno-omit-frame-pointer
```

Options (`CMakeLists.txt:10-14`, defaults `OFF` except `CRUX_TAGGED_OBJECT=ON`):
- `CRUX_STACK_SAFETY`, `CRUX_DEBUG_TRACE_EXECUTION`, `CRUX_DEBUG_PRINT_CODE`, `CRUX_DEBUG_LOG_GC`, `CRUX_DEBUG_STRESS_GC`
- `CRUX_TAGGED_OBJECT` — 48-bit pointer tagging (`src/_headers/object/object.h:144`), disable on 32-bit (`UINTPTR_MAX == 0xFFFFFFFF`).
- `CRUX_VERSION` cache string `dev` → `-DCRUX_VERSION=0.22.0` for releases.

Artifacts (`CMakeLists.txt:77`):
- `libcrux.a` (`crux_lib` STATIC) — link via `target_link_libraries(your_app PRIVATE crux_lib m)` `CMakeLists.txt:81`
- `crux` CLI (`src/cli/main.c` + `deps/json/cJSON.c` + `deps/linenoise/linenoise.c` on Unix)
- `crux_embedding_example` (`examples/embedding/main.c` + `src/file_handler.c`)

## Verify

```bash
./build/crux --version
./build/crux_embedding_example # C: Result from Crux was 42
CRUX_STDLIB=std ./build/crux std/_std_test/collections_test.crux # All collection tests passed
cd tests && python test_runner.py # All Tests Ran Successfully
```

Next: [First Program](first-program.md)
