# Functions `src/compiler/compiler_functions.c:12` `src/_headers/compiler/compiler_functions.h`

```crux
fn add(a: Int, b: Int) -> Int { return a + b; }
pub fn hello(name: String) -> String { return "hi " + name; }
var f: (Int)->Int = fn (x: Int) -> Int { return x*2; };
```

- `fn name(params:Type) -> Ret { }` `compiler_functions.c:12` `parse type` `compiler_core.c:23` `Ret` default `Nil`, arity ≤255 `NATIVE_FUNCTION_MAX_ARGS=8` for natives `object.h:15` but Crux fn up to 255.
- `pub fn` top-level only `consume pub` `compiler_core.c:54` exposed `crux_get_variable`.
- Anonymous `fn (a:Int,b: String)->Bool { }` → `OP_ANON_FUNCTION` `compiler_expressions.c:412` `ObjectClosure`.
- Closures capture `count` `tests/features/functions.crux:214` via `ObjectUpvalue` `object.h:263` `location/closed/next` `open_upvalues` `vm.h:125`.
- Higher-order:

```crux
fn apply(f: (Int)->Int, x: Int) -> Int { return f(x); }
apply(fn (n:Int)->Int { return n*2; }, 5); // 10
var counter = 0; var inc = fn (){ counter+=1; return counter; }; inc(); // 1
```

- Recursion via `recursive_name` `compiler_core.c:541` hoisted before body.

No overloading; use `Any` for generic `fn foo(x: Any)`.

See [Structs](structs.md) methods, [Types](types.md) `(A,B)->Ret`, [Control Flow](control-flow.md) `return`.
