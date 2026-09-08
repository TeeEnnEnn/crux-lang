# Crux 0.22 Status

Crux is a young, gradually typed, garbage-collected bytecode interpreter. The public
language and embedding APIs are usable, but neither is stable yet. Code written for 0.22
may require changes in later 0.x releases.

## Supported today

- `Nil`, `Bool`, 32-bit `Int`, double-precision `Float`, and UTF-8 `String` values.
- Arrays, tables, tuples, ranges, buffers, vectors, matrices, and complex numbers.
- Optional annotations, union types, nominal structs, structural shapes, type aliases,
  explicit casts, `typeof`, and flow-sensitive narrowing.
- Named and anonymous functions, closures, recursion, methods, and static methods.
- `if`, `while`, C-style `for`, `for ... in`, `break`, `continue`, and expression `match`.
- `Result[T]`, `Option[T]`, the `?` unwrap operator, `panic`, and runtime stack traces.
- File imports, built-in `crux:` modules, installed `pkg:` dependencies, and source
  `std:` packages.
- A CLI, source package installer, and C embedding API.

The native library inventory is in the [standard-library index](stdlib/README.md). Crux
source packages currently consist of [collections](stdlib/collections.md) and
[statistics](stdlib/statistics.md).

## Platforms and builds

The project builds as C11 with GCC and CMake 3.28 or newer. Release automation produces
Linux amd64, Windows amd64, macOS amd64, and macOS arm64 artifacts. Other Unix-like
platforms may work from source but are not release targets.

AddressSanitizer builds are restricted to Unix-like systems. Tagged object pointers are
enabled by default and must be disabled on 32-bit targets.

## Stability

There is currently no formal compatibility guarantee for:

- Source syntax, type-checking details, or standard-library signatures.
- Bytecode opcodes and object layouts.
- The package manifest and dependency installation behavior.
- The C ABI beyond a single matching Crux build.

Pin the Crux release used by an application and rebuild native hosts against that release.
Serialized bytecode is not a supported distribution format.

## Known limitations

- Array `sort()` is registered but its test is skipped because ordering is not currently
  reliable. The statistics package's `median()` function depends on it.
- Generic type aliases are not implemented.
- `Int` is limited to signed 32-bit values. Numeric overflow behavior is not yet a stable
  language contract.
- Only line comments (`//`) are supported; there are no block comments.
- The package installer clones Git URLs but has no lockfile, resolver, update command,
  checksums, or package registry.
- Native `crux:` modules are compiled into the executable. There is no dynamic native
  package loader.
- The public header contains foreign-class callback types, but host-owned foreign objects
  and finalization are not implemented end to end.
- The VM has no documented thread-safety guarantee. Treat a VM as confined to one thread.
- The C slot API exposes only a subset of object kinds through `CruxType`.
- Imported-file standard-library behavior still has an unresolved test TODO.

See the [embedding limitations](embedding/README.md#current-limitations) for host API
details and [Testing](contributor/testing.md) for the currently exercised behavior.

## Reporting gaps

When implementation, tests, and documentation disagree, tests and implementation describe
the current behavior. Please report the discrepancy and include a minimal `.crux` program.
Documentation examples intended as executable checks live in `docs/examples/` and are
validated by `scripts/check_docs.py`.
