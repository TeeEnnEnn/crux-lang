# Complex `src/native/complex.c` `ObjectComplex` `object.h:473` `vm.complex_type:473`

Constructor `Complex(Numeric,Numeric)->Complex` `native_registration.c:489`:

```crux
var c = Complex(1.0,2.0);
```

## Methods `Complex.` 10 `native_registration.c:475`

| Method | Signature | Return |
|--------|-----------|--------|
| `add/sub/mul/div` | `(Complex,Complex)` | `Complex` |
| `scale` | `(Complex,Numeric)` | `Complex` |
| `real/imag` | `(Complex)` | `Float` |
| `conjugate` | `(Complex)` | `Complex` |
| `mag/square_mag` | `(Complex)` | `Float` |

```crux
use Complex from "crux:complex";
var a = Complex(1,2); var b = Complex(3,4);
var sum = a.add(b); // (4,6)
println(string(sum.real())); // 4.0
```

Tests `tests/modules/complex.crux` + `benchmarks/mandelbrot.crux` `square_and_add`.

See [Math](math.md) `Complex` as numeric.
