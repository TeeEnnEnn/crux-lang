# Testing `tests/test_runner.py:5` + `std/_std_test/test_runner.py:1` + `benchmarks`

## `tests/test_runner.py:5`

```python
EXE_PATH="../build/crux"
directories=["builtins","features","modules","importing","compiler"]
for d in directories:
  for f in os.listdir(f"./{d}/"):
    subprocess.run([EXE_PATH,f])
```

Covers 16 `features` (`for_in`, `match`, `collection_slicing` `tests/features/*.crux`) + 16 `modules` (`array`, `buffer` `tests/modules/*.crux`) + 3 `compiler` (`basic_types`, `flow_typing`, `shapes`) + 2 `importing` + 2 `builtins`.

## `std/_std_test/test_runner.py:1`

```python
EXE_CANDIDATES=["../../build/crux",...]
CRUX_STDLIB=str(pathlib.Path(__file__).parent.parent)
files=[f for f in os.listdir(".") if f.endswith(".crux")]
subprocess.run([EXE_PATH,file],env=env)
```

Runs `collections_test.crux` 121 lines + `stats_test.crux` via `CRUX_STDLIB=std`.

## Benchmarks

`tests/benchmarks/*.crux` (`cpu_stress`, `mandelbrot`, `ray_tracer`, `terrain_gen`, `xor_ai`) + `gc_harness.py` + `analyze_gc.R` manual.

## CI `run_test.yml:121`

`cd tests && python test_runner.py` + `cd std/_std_test && python test_runner.py` `env: CRUX_STDLIB=${{github.workspace}}/std` + `Run Embedding Example` `./build/crux_embedding_example`.

## Add Test

Create `tests/features/my.crux` with `assert`, add dir to `test_runner.py` if new category. Local: `cmake -S . -B build && cmake --build build && python tests/test_runner.py`.

## Documentation

Run documentation link checks, native and source-package API coverage checks, and the
executable examples with:

```bash
python3 scripts/check_docs.py --require-runtime
```

The checker discovers `build/crux` automatically. Use `--crux path/to/crux` for another
build. Add complete programs to `docs/examples/`; snippets that intentionally demonstrate
errors remain inline in their reference pages.
