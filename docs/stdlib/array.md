# Array `src/native/array.c` `ObjectArray` `object.h:277` `vm.array_type:313`

Literal `[1,2,3]` `OP_ARRAY` `chunk.h:6`, `Array[T]` `value.h:47`.

## Methods `Array[Any].` 16 `native_registration.c:313`

| Method | Signature | Return | Notes |
|--------|-----------|--------|-------|
| `push` | `(Array[Any],Any)` | `Result[Nil]` | `array_add_back` |
| `pop` | `(Array[Any])` | `Option` | `Some` or `None` |
| `insert` | `(Array[Any],Int,Any)` | `Result[Nil]` | at index |
| `remove` | `(Array[Any],Int)` | `Result[Any]` | remove at |
| `clear` | `(Array[Any])` | `Nil` |  |
| `concat` | `(Array[Any],Array[Any])` | `Result[Array[Any]]` |  |
| `slice` | `(Array[Any],Int,Int)` | `Result[Array[Any]]` | `OP_GET_SLICE` |
| `reverse` | `(Array[Any])` | `Result[Nil]` | in-place |
| `index` | `(Array[Any],Any)` | `Option[Int]` | first |
| `contains` | `(Array[Any],Any)` | `Bool` |  |
| `equals` | `(Array[Any],Array[Any])` | `Bool` |  |
| `map` | `(Array[Any],(Any)->Any)` | `Result[Array[Any]]` | higher-order |
| `filter` | `(Array[Any],(Any)->Bool)` | `Result[Array[Any]]` | callback must return `Bool`; current native metadata accepts `Any` |
| `reduce` | `(Array[Any],(Any,Any)->Any,Any)` | `Result[Any]` |  |
| `sort` | `(Array[Any])` | `Result[Array[Any]]` |  |
| `join` | `(Array[Any],String)` | `Result[String]` |  |

```crux
var arr = [1,2,3];
arr.push(4)?;
var v = (arr.pop() as Option[Any]).unwrap(); // 4
var mapped = arr.map(fn (x){return x*2;})?; // [2,4,6]
for var x in arr { println(string(x)); } // iterable
```

Core `len(Any)->Int` `native_registration.c:258`.

Tests `tests/modules/array.crux` + `tests/features/collection_literals.crux`.

See [Iterators](../language/iterators.md), [Slices](../language/slices.md).
