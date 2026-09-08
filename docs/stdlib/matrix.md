# Matrix `src/native/matrix.c` `ObjectMatrix` `object.h:479` `vm.matrix_type:498`

Constructors `use Matrix, IMatrix, AMatrix from "crux:matrix"` `native_registration.c:523`:

| Ctor | Signature | Return |
|------|-----------|--------|
| `Matrix` | `(Int rows,Int cols)` | `Result[Matrix]` |
| `IMatrix` | `(Int dim)` | `Result[Matrix]` — identity |
| `AMatrix` | `(Int rows,Int cols,Array[Numeric])` | `Result[Matrix]` |

## Methods `Matrix[,].` 19 `native_registration.c:499`

| Method | Signature | Return |
|--------|-----------|--------|
| `get/set` | `(Matrix,Int,Int[,Numeric])` | `Result[Float]/Result[Nil]` |
| `add/sub/mul` | `(Matrix,Matrix)` | `Result[Matrix]` |
| `scale` | `(Matrix,Numeric)` | `Matrix` |
| `transpose/determinant/inverse/trace/rank` | `(Matrix)` | `Matrix/Result[Float/Int]` |
| `row/col` | `(Matrix,Int)` | `Result[Vector]` |
| `equals` | `(Matrix,Matrix)` | `Result[Bool]` |
| `copy/to_array/mul_vec/rows/cols` | `(Matrix[,Vector])` | `Result[Array]/Vector/Int` |

`Matrix[2,3]` vs `Matrix[,]` wildcard `types.md`.

```crux
use Matrix from "crux:matrix";
var m = Matrix(2,2)?; // 2x2 zero
var id = IMatrix(2)?;
println(string(id.get(0,0)?)); // 1.0
```

Tests `tests/modules/matrix.crux` + `benchmarks/cpu_stress.crux` `mat_mul`.
