# Declarations and Scope

## Variables

`var` declares a mutable binding:

```crux
var inferred = 1;
var checked: Int = 2;
checked = 3;
```

An initializer must be compatible with an annotation. Reassignment is checked against the
binding's declared or inferred type. A block creates a nested lexical scope. Inner
bindings may shadow outer bindings; use distinct names when shadowing would obscure which
value a closure captures.

## Functions and closures

Named functions are visible in their module and may recurse. A closure keeps captured
locals alive after the defining scope exits.

```crux
fn counter() -> () -> Int {
  var value = 0;
  return fn () -> Int {
    value += 1;
    return value;
  };
}
```

## Module scope and publication

Top-level declarations are private unless marked `pub`:

```crux
pub var version = "0.22";
pub fn run() -> Nil {}
pub struct Configuration { enabled: Bool }
```

`pub` is only meaningful at module scope. Other modules can import public names with
`use`; `pub use` re-exports an imported public name. A typical package exposes its public
surface from `pkg.crux`.

## Declaration order

The precompiler discovers public structs and functions before full compilation, allowing
common forward references. Do not rely on arbitrary execution-order access to uninitialized
variables; initialize module state before code that reads it.

See [Variables](variables.md), [Functions](functions.md), and [Imports](imports.md).
