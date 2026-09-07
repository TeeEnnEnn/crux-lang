# Language Overview

Crux is gradually typed, memory-managed, interpreted. Every value is a `CruxValue` `include/crux.h:38` (NaN-boxed `uint64_t` when `CRUX_TAGGED_OBJECT` `src/_headers/object/object.h:144` else struct). Objects (`ObjectType`: `STRING … OPTION` `object.h:104`) are GC roots via `CruxHandle` or stack.

## Values vs Objects

* Primitive `CruxValue`: `nil`, `Bool`, `Int` (`int32_t`), `Float` (`double`), tagged via `CRUX_QNAN` `crux.h:54`.
* Objects: `Array`, `Table`, `String`, `Buffer`, `Vector`, `Matrix`, `Complex`, `Range`, `Tuple`, `StructInstance`, `Closure`, `Error`, `Result`, `Option`, `File`, `Random`.

## Gradual Typing

Annotations optional (`var x = 5` → `T_ANY` unless inferred). `types_compatible` `src/type_system/type_system.c:322`:
- `Any` ↔ everything, `Never` bottom (`panic`/`return`/`break`).
- `Int` → `Float` (promotion), `Vector[3]` → `Vector[]` (`-1` wildcard).
- Structural `shape` vs nominal `struct` (`types_equal` checks `Struct.name` identity).
- Union `Int|String` order-independent.

Example:
```crux
var a: Any = 5;
a = "now string";
var u: Int|String = 5;
u = "hi"; // u = 3.14 // Type Error
var s: shape {x:Int} = new Point {x=1,y=2}; // ok structural
```

## Execution Model

`scanner.c` → `pre_compiler.c` (forward `pub struct`/`pub fn`) → `compiler_*` (`compiler_core.c:23` `parse_type_record`) → `Chunk` `src/_headers/chunk.h:6` bytecode → `vm_run.c` dispatch loop `CallFrame` `vm.h:125` (`stack/stack_top/frames`).

See [Syntax](syntax.md), [Types](types.md), [Control Flow](control-flow.md).
