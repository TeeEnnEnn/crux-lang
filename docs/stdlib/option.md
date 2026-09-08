# Option `src/native/option.c` `ObjectOption` `object.h:332` `vm.option_type:370`

Created via `Some(val)`/`None`, `array.pop()->Option`, `table.get` + `next(iter)->Option`.

## Methods `Option[T].` 4 `native_registration.c:370`

| Method | Signature | Return |
|--------|-----------|--------|
| `unwrap` | `(Option[T])` | `T` — panics if `None` |
| `unwrap_or` | `(Option[T],T)` | `T` |
| `is_some` | `(Option[T])` | `Bool` |
| `is_none` | `(Option[T])` | `Bool` |

```crux
var arr = [1,2];
var maybe: Option[Int] = arr.pop(); // Some(2)
var v = maybe.unwrap(); // 2
var empty: Option[Int] = ([] as Array[Int]).pop(); // None
println(string(empty.is_none())); // true
var m = match maybe {
  Some(val) => { give val; }
  None => { give 0; }
};
```

Tests `tests/features/result_option_methods.crux` + `tests/modules/array.crux` `pop`.

See [Result](result.md) `Result` vs `Option`.
