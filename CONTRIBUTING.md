# Contributing to Crux

Thanks for considering contributing.

**Project status:** Crux isn't under active development right now — I've pressed pause on the project for now. I got tired of working on it alone. Issues and pull requests are welcome and are just the thing that I would need to get back into working on Crux.

## Before you start

Read the docs.

1. [`src/README.md`](src/README.md) — the contributor entry point. It maps source directories (compiler, VM, GC, type system, natives, CLI) to
   the pages in the docs that explain them in more detail.
2. [`docs/README.md`](docs/README.md) — the full documentation starting point.

If you're planning something non-trivial, please open an issue first.

## Building

**Linux**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make
```

**macOS** — same as Linux.

**Windows** — install [mingw-w64](https://www.mingw-w64.org/) and make sure
`gcc` is on your `PATH`, then:
```shell
cmake -G "MinGW Makefiles" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

For a debug build (assertions, no optimization) use
`-DCMAKE_BUILD_TYPE=Debug` instead of `Release`. Full list of CMake flags
(`CRUX_STACK_SAFETY`, `CRUX_DEBUG_TRACE_EXECUTION`, etc.) is in
[`docs/tools/build.md`](docs/tools/build.md).

## Running tests

```bash
python tests/test_runner.py
```

This runs everything under `tests/` (`.crux` files exercising language
features, builtins, modules, and the compiler) against `build/crux`. To also
run the standard library tests:

```bash
cd std/_std_test && CRUX_STDLIB=../../std python test_runner.py
```

If you're adding a feature, add a `.crux` file under the matching
`tests/` subdirectory that asserts the behavior. If you're adding a new
test category, register the new directory in `tests/test_runner.py`.

## Code style

Formatting is enforced by `.clang-format`.

```bash
clang-format -i path/to/file.c
```

## Commit messages

Release notes are generated automatically from commit messages, so the
format actually matters functionally, not just cosmetically. Use
[Conventional Commits](https://www.conventionalcommits.org/):

- `feat: add pattern matching for tuples` — new feature
- `fix: correct GC mark phase for closures` — bug fix
- `feat!: change stdlib import syntax` — breaking change (note the `!`)
- anything else (`docs:`, `refactor:`, `chore:`, `test:`) goes in "Other
  Changes" in the changelog

## Submitting a change

1. Fork the repo and branch off `main`.
2. Make your change.
3. Run `clang-format` and the test suite locally.
4. Open a PR with a short description of what changed and why.

## Reporting bugs

Open an issue with: what you expected, what happened instead, and a minimal
`.crux` snippet that reproduces it if possible.

## License

Crux is MIT licensed. 
By contributing, you agree your contributions are licensed under the same terms.
