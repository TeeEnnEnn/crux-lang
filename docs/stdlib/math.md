# Math `src/native/math.c` module `math` `native_registration.c:625`

Fns `use ... from "crux:math"`:

| Fn | Signature | Return |
|----|-----------|--------|
| `pow` | `(Numeric,Numeric)` | `Float` |
| `sqrt` | `(Numeric)` | `Result[Float]` |
| `ceil/floor/round` | `(Numeric)` | `Int` |
| `abs` | `(Numeric)` | `Numeric` |
| `sin/cos/tan/atan` | `(Numeric)` | `Float` |
| `acos/asin` | `(Numeric)` | `Result[Float]` |
| `exp/ln/log` | `(Numeric)` | `Float/Result[Float]` |
| `min/max` | `(Numeric,Numeric)` | `Numeric` |
| `e/pi/nan/inf` | `()` | `Float` |

```crux
use sin, sqrt, pi from "crux:math";
println(string(sin(pi()/2))); // 1.0
var r = sqrt(4)?; // 2.0
```

Tests `tests/modules/math.crux` + `benchmarks/*`.

See [Core](core.md) `Numeric` `INT|FLOAT`.
