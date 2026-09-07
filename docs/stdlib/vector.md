# Vector `src/native/vectors.c` + `vector_helpers.c` `ObjectVector` `object.h:464` `vm.vector_type:440`

Constructors `use Vector from "crux:vector"`:

| Ctor | Signature | Return |
|------|-----------|--------|
| `Vector` | `(Int dim, Array[Numeric])` | `Vector[]` — `native_registration.c:464` |
| — | — | `Vector[3]` vs `Vector[]` `-1` wildcard `type_system.c:212` |

## Methods `Vector[].` 18 `native_registration.c:440`

| Method | Signature | Return |
|--------|-----------|--------|
| `dot/add/subtract` | `(Vector,Vector)` | `Result[Float]/Result[Vector]` |
| `multiply/divide` | `(Vector,Numeric)` | `Result[Vector]` |
| `magnitude/normalize` | `(Vector)` | `Float/Result[Vector]` |
| `distance/angle_between/cross/lerp/reflect` | `(Vector,Vector[,Numeric])` | `Result[Float]/Result[Vector]` |
| `equals` | `(Vector,Vector)` | `Bool` |
| `x/y/z/w` | `(Vector)` | `Float` |
| `dimension` | `(Vector)` | `Int` |

```crux
use Vector from "crux:vector";
var v = Vector(3,[1.0,2.0,3.0])?;
println(string(v.magnitude())); // 3.74
var u = v.normalize()?; // unit
```

Tests `tests/modules/vectors.crux`.

See [Matrix](matrix.md) `Vector` interop, [Types](../language/types.md) `Vector[3]`.
