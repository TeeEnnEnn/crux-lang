# Random `src/native/random.c` `ObjectRandom` `object.h:284` `vm.random_type:399`

Module `random` `native_registration.c:411` `Random()`:

```crux
use Random from "crux:random";
var rng = Random();
rng.seed(42);
```

## Methods

| Method | Signature | Return |
|--------|-----------|--------|
| `seed` | `(Random,Int)` | `Nil` |
| `int` | `(Random,Int,Int)` | `Result[Int]` — inclusive |
| `float` | `(Random,Numeric,Numeric)` | `Result[Float]` |
| `bool` | `(Random,Numeric)` | `Result[Bool]` |
| `choice` | `(Random,Array[Any])` | `Result[Any]` |
| `next` | `(Random)` | `Float` — 0..1 |

```crux
var n = rng.int(0,10)?; // 0..10
var f = rng.float(0.0,1.0)?; // 0.0..1.0
var pick = rng.choice([1,2,3])?; // random element
```

`seed` uses `srand` `object.h:588`.

Tests `tests/modules/random.crux`.
