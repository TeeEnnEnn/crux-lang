# Variables `src/compiler/compiler_declarations.c:58` `src/scanner.c:325` `CRUX_TOKEN_VAR`

```crux
var x = 5;               // inferred Int via literal
var y: Int = 5;          // annotated
var z: String = "hi";
var any: Any = 5; any = "now string"; // Any↔any
pub var global: Int = 42; // top-level only
```

- `var` (was `let` pre `44abb60` `b93c5be` for `var`), `CRUX_TOKEN_VAR` `scanner.c:325`.
- Local inside `fn`/`for`/`if` block `begin_scope` `compiler_declarations.c:58` vs global top-level `vm.global_names` `vm.h:125` `module_record->globals`.
- `pub var` only top-level, exposed `crux_get_variable(vm,module,name,slot)` `crux.h:54` `examples/embedding/main.c:77`.
- Must initialize non-`Nil` `if (type != T_NIL && !initializer) error` `compiler_declarations.c:58`.
- Reassignment `x=6;` `types_compatible` `type_system.c:212` — `Int|String` allows both, else `Type Error` `basic_types.crux:24`.
- Shadowing `for var i=0` creates new scope `common.h:12` `STACK_MAX`.

```crux
var outer=100;
for var inner=0; inner<3; inner+=1 { println(string(inner)); } // 0,1,2
println(string(outer)); // 100 unchanged
```

See [Types](types.md), [Control Flow](control-flow.md), [Functions](functions.md).

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
