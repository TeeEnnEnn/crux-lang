# crux-lang

A simple gradually typed, memory-managed, interpreted programming language that let's you get to the **crux** of your code.

**Docs:** All documentation lives in [`docs/`](docs/README.md) — see [Getting Started](docs/getting-started/installation.md) + [Language Overview](docs/language/overview.md) + [Stdlib](docs/stdlib/README.md) + [Embedding](docs/embedding/lifecycle.md). Contributor entry is [`src/README.md`](src/README.md).

## Getting Started

To get started you can download the latest Crux release from the [release page](https://github.com/TheophilusNenhanga/crux-lang/releases), or you can build from source. 

### Building from source

#### Windows

To build from source on Windows, you need to have mingw (for gcc) installed, and in your PATH.
You can get mingw [here](https://www.mingw-w64.org/).

```shell
cmake -G "MinGW Makefiles" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Linux

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make
```
