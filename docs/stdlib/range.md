# Range `src/native/range.c` `ObjectRange` `object.h:523` `vm.range_type:534`

Constructor `Range(Int start,Int end,Int step)->Result[Range]` `native_registration.c:546` or literal `1..10` `tests/features/range_literals.crux` (`1..1..10` step).

Validation `validate_range_values` `object.h:588` (`step !=0`).

## Methods `Range.` 7 `native_registration.c:534`

| Method | Signature | Return |
|--------|-----------|--------|
| `contains` | `(Range,Int)` | `Bool` |
| `to_array` | `(Range)` | `Result[Array[Int]]` |
| `start/end/step` | `(Range)` | `Int` |
| `is_empty` | `(Range)` | `Bool` |
| `reversed` | `(Range)` | `Range` |

```crux
var r = Range(0,10,2)?; // 0,2,4,6,8
for var x in r { println(string(x)); }
println(string(r.contains(4))); // true
var arr = r.to_array()?; // [0,2,4,6,8]
```

Iterable (`is_iterable_type` `type_system.c:212`) `for var x in r`.

Tests `tests/modules/range.crux`.
