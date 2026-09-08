# First Program

## Hello

`hello.crux`:
```crux
println("Hello, Crux!");
```

Run:
```bash
crux hello.crux
# or
./build/crux hello.crux
```

REPL:
```bash
crux
> println("hi");
> var x: Int = 5;
> println(string(x));
> :exit # Ctrl-D
```
History: `~/.local/share/crux/repl_history` Unix (`src/cli/cli_commands.c:94` via `linenoise`), no `linenoise` on Windows.

## Project: `crux init`

```bash
crux init myapp
tree myapp
# myapp/
# ├── crux.json       # manifest
# ├── main.crux       # entry
# ├── pkg.crux        # re-exports
# └── crux_modules/   # after `crux install`

cat myapp/crux.json
# {
#   "name": "myapp",
#   "version": "0.1.0",
#   "main": "main.crux",
#   "dependencies": {
#     "math": "https://github.com/user/crux-math.git"
#   }
# }

cat myapp/main.crux
# use greet from "./pkg.crux";
# println(greet("world"));

cat myapp/pkg.crux
# pub use greet from "./greet.crux";
```

Run project:
```bash
cd myapp
crux run          # uses crux.json "main", falling back to main.crux
# From the parent directory, pass the script rather than the project directory:
crux myapp/main.crux
```

Install deps: `crux install` clones git deps into `crux_modules/` recursively (`src/file_handler.c:195` upward search bounded by manifest).

## std:collections quick taste

```crux
use Stack, Set from "std:collections";
var s = new Stack {data = []};
s.push(1); s.push(2);
var v = (s.pop() as Option[Any]).unwrap(); // 2

var set = new Set {data = {}};
set.add(1); set.add(2);
println(string(set.contains(1))); // true
```

```crux
use mean from "std:statistics";
println(string(mean([1.0,2.0,3.0]))); // 2.0
```

Next: [Project Layout](project-layout.md) — `crux.json`/`pkg.crux`/`crux_modules/` + `std:`/`pkg:` resolution.
