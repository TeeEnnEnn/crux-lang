# CI `.github/workflows/release.yml:1` `run_test.yml:1`

## `release.yml` Create Release `workflow_dispatch` `version` input `release.yml:4`

Matrix 4 OS `release.yml:15`: `ubuntu linux amd64`, `windows amd64 MSYS Makefiles`, `macos amd64` (`-DCMAKE_OSX_ARCHITECTURES=x86_64`), `macos arm64` (`arm64`), `cmake -DCRUX_STACK_SAFETY=OFF -DCRUX_VERSION=...` `release.yml:25`.

Steps `release.yml:85`: `Rename binary` `crux-{platform}-{arch}[.exe]`, `Upload artifact`, `Create Stdlib Archives` `release.yml:101` `cp -r std/* dist/stdlib/` → `tar -czf -C dist stdlib` + `zip`, `Prepare Assets` `cp scripts/install/install.sh/.ps1` `find . -maxdepth 2 -name "crux-*"`, `Generate Release Notes` conventional commits `feat`→Feature `fix`→Bug Fix `!`→Breaking (`release.yml:48`), `softprops/action-gh-release` `tag v${{github.event.inputs.version}}` files `crux-*` `install.sh/ps1` + manual `Downloads` section `release.yml:206`.

Env `contents: write` `release.yml:5` for release.

## `run_test.yml` run crux test suite `run_test.yml:1`

Triggers `pull_request` `main` paths `**.c/h/crux`+`std/**` `run_test.yml:14`, `workflow_dispatch` `branch` input `run_test.yml:3`. Same matrix, `brew install gcc` macOS `msvc/setup-msys2@v2` Windows `run_test.yml:76`, `Build Linux/macOS` `cmake -DCMAKE_BUILD_TYPE=Release` `run_test.yml:93`, `Run tests` `cd tests && python test_runner.py` `tests/test_runner.py:5` `EXE_PATH="../build/crux"`, `Run stdlib tests` `cd std/_std_test && python test_runner.py` `env: CRUX_STDLIB=${{github.workspace}}/std CRUX_EXE=${{github.workspace}}/build/${{matrix.binary_name}}` `run_test.yml:126`, `Run Embedding Example` `./build/crux_embedding_example`.

Local repro: `mkdir build && cmake -DCMAKE_BUILD_TYPE=Release -S .. && cmake --build . && python tests/test_runner.py && CRUX_STDLIB=std python std/_std_test/test_runner.py`.

Conventional commits required for changelog `feat`/`fix`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
