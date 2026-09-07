# Tuple `src/native/tuple.c` `ObjectTuple` `object.h:538` `vm.tuple_type:556`

Constructor `Tuple(Array[Any])->Tuple[]` `native_registration.c:571`:

```crux
var t = Tuple([1,"a",3.14]);
```

## Methods

| Method | Signature | Return |
|--------|-----------|--------|
| `get` | `(Tuple,Int)` | `Result[Any]` |
| `slice` | `(Tuple,Int,Int)` | `Result[Array]` |
| `index` | `(Tuple,Any)` | `Option[Int]` |
| `is_empty` | `(Tuple)` | `Bool` |
| `to_array` | `(Tuple)` | `Array` |
| `first/last` | `(Tuple)` | `Option` |
| `contains` | `(Tuple,Any)` | `Bool` |
| `equals` | `(Tuple,Tuple)` | `Bool` |

```crux
use Tuple from "crux:tuple";
var t = Tuple([1,2,3]);
println(string(t.get(0)?)); // 1
println(string(t.slice(0,2)?)); // [1,2]
```

Tests `tests/modules/tuple.crux` + `compiler/basic_types.crux` `Tuple[Int]`.

See [Types](../language/types.md) `Tuple[Int,String]`.
