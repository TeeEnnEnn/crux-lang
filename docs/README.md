# Crux Documentation

> Gradually typed, memory-managed, interpreted language. All docs live in this `docs/` tree and render natively on GitHub — no external site.

## Quick Links

- **First run in 30 seconds** → [Getting Started: Installation](getting-started/installation.md) → [First Program](getting-started/first-program.md)
- **What works today** → [Language Status](status.md)
- **Write Crux** → [Language Overview](language/overview.md)
- **Project layout & imports** → [Project Layout](getting-started/project-layout.md)
- **Standard library** → [Stdlib Index](stdlib/README.md)
- **Embed Crux in C** → [Embedding Lifecycle](embedding/lifecycle.md) ([example](../examples/embedding/main.c))
- **CLI reference** → [Tools: CLI](tools/cli.md)
- **Contributor build** → [Build](tools/build.md) + [Architecture](../src/README.md)

## Contents

### Getting Started
- [Installation](getting-started/installation.md) — `install.sh`/`install.ps1`, `CRUX_INSTALL_DIR`, source build matrix
- [First Program](getting-started/first-program.md) — `hello.crux`, `crux init/run`, REPL tour
- [Project Layout](getting-started/project-layout.md) — `crux.json` + `pkg.crux` `pub use`, `crux_modules/` recursion, `std:`/`pkg:` resolution

### Language (full reference)
- [Current Status](status.md) — supported features, stability, platforms, and known limitations
- [Overview](language/overview.md) — values vs objects, gradual typing, memory model
- [Syntax](language/syntax.md) — lexical structure, comments, identifiers, and punctuation
- [Literals](language/literals.md) — primitive and collection literals
- [Operators](language/operators.md) — precedence, operand types, and behavior
- [Scope](language/scope.md) — declarations, shadowing, publication, and module scope
- [Types](language/types.md) — `TypeMask` `src/_headers/value.h:47`, generics `Array[T]` `Table[K,V]` `Vector[3]` `Matrix` `Tuple` `Result` `Option` `Any/Never/Nil`
- [Variables](language/variables.md) — `var`, `pub var`, annotations
- [Functions](language/functions.md) — `fn`, closures, anonymous `fn`
- [Structs](language/structs.md) — `struct`/`new`/`shape`
- [Impl](language/impl.md) — `impl`, `fn` vs `static fn`, `::`
- [Type Aliases](language/type-aliases.md) — `type My = Int|String`
- [Control Flow](language/control-flow.md) — `if/while/for/for-in/break/continue/panic`
- [Match](language/match.md) — `match` exhaustiveness, `give`, `Ok/Err` `Some/None`
- [Error Handling](language/error-handling.md) — `Result`/`Option`/`Error`, `?`, `match`
- [Imports](language/imports.md) — `use`/`pub use`/`as`/`from`, `crux:` vs file
- [Iterators](language/iterators.md) — `__iter`/`__next` → `Option`
- [Slices](language/slices.md) — `Range`, `arr[1..3]`

### Standard Library (per module)
- [Index](stdlib/README.md) + [Core](stdlib/core.md), [String](stdlib/string.md), [Array](stdlib/array.md), [Table](stdlib/table.md), [Result](stdlib/result.md), [Option](stdlib/option.md), [Error](stdlib/error.md), [File](stdlib/file.md), [Random](stdlib/random.md), [GC](stdlib/gc.md), [Vector](stdlib/vector.md), [Complex](stdlib/complex.md), [Matrix](stdlib/matrix.md), [Range](stdlib/range.md), [Tuple](stdlib/tuple.md), [Buffer](stdlib/buffer.md), [Math](stdlib/math.md), [IO](stdlib/io.md), [Time](stdlib/time.md), [Sys](stdlib/sys.md), [FS](stdlib/fs.md), [Collections](stdlib/collections.md), [Statistics](stdlib/statistics.md)

### Embedding
- [Lifecycle](embedding/lifecycle.md) — 11-step checklist
- [Slots](embedding/slots.md) — `crux_ensure_slots`, `set/get_slot_*`
- [Handles](embedding/handles.md) — `CruxHandle`
- [Foreign Functions](embedding/foreign-functions.md) — `native fn` + `bindForeignMethodFn`
- [Modules](embedding/modules.md) — `resolveModuleFn`/`loadModuleFn`
- [GC Tuning](embedding/gc-tuning.md)
- [Examples](embedding/examples.md) — link to [examples/embedding/main.c](../examples/embedding/main.c)

### Tools
- [CLI](tools/cli.md) — `crux`, `run/init/install`, `--version`, exit codes
- [Build](tools/build.md) — `CMakeLists.txt` flags, build types, tagged objects
- [Install Scripts](tools/install-scripts.md) — `install.sh`/`install.ps1`, `CRUX_BASE_URL`
- [CI](tools/ci.md) — `release.yml`/`run_test.yml`, conventional commits

### Contributor
- [Architecture](../src/README.md) — starting point (see also [docs/contributor/architecture.md](contributor/architecture.md))
- [Compiler](contributor/compiler.md) — `scanner→pre_compiler→compiler_*`
- [VM](contributor/vm.md) — opcodes `src/_headers/chunk.h:6`
- [Type System](contributor/type-system.md)
- [GC](contributor/gc.md) — slab allocators, `gc_last_*`
- [Native](contributor/native.md) — add `src/native/*.c`
- [Testing](contributor/testing.md) — `tests/test_runner.py:5` + `std/_std_test`
- [Debugging](contributor/debugging.md) — `DEBUG_TRACE_EXECUTION`

### Reference (full depth)
- [Tokens](reference/tokens.md) — `scanner.h` enum
- [Opcodes](reference/opcodes.md) — `chunk.h` tables
- [Type Masks](reference/type-masks.md) — `value.h`
- [Object Types](reference/object-types.md) — `object.h`
- [Native API](reference/native-api.md) — `include/crux.h` table

Implementation links are secondary references. Language and library pages describe the
public behavior first; internal details belong in the contributor and reference sections.
