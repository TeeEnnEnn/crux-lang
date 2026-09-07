# Result `src/native/result.c` `ObjectResult` `object.h:324` `vm.result_type:359`

Fallible natives return `Result[T]` (`RES` `native_registration.c:88`):

```crux
var r: Result[Int] = int("5"); // Ok(5)
var value: Int = r?;           // unwrap; panic if r is Err
var e: Result[Int] = int("bad"); // Err(Error)
```

## Methods `Result[T].` 4 `native_registration.c:359`

| Method | Signature | Return |
|--------|-----------|--------|
| `unwrap` | `(Result[T])` | `T` — panic if `Err` |
| `unwrap_or` | `(Result[T],T)` | `T` |
| `is_ok` | `(Result[T])` | `Bool` |
| `is_err` | `(Result[T])` | `Bool` |

`?` is a postfix unwrap operator. It returns the `Ok` value and raises a runtime panic for
`Err`; it does not propagate an `Err` to the caller:

```crux
fn parse(s:String)->Result[Int] {
  var parsed = int(s);
  if parsed.is_err() { return Err(error("invalid integer")); }
  var v = parsed?;
  return Ok(v);
}
var out = match int("5") {
  Ok(v) => { give v; }
  Err(err) => { give 0; }
};
```

Creation `Ok(val)` + `Err(error)` core + `MAKE_GC_SAFE_ERROR/RESULT` `object.h:20`.

Tests `tests/features/result_option_methods.crux`.

See [Error](error.md), [Option](option.md).
