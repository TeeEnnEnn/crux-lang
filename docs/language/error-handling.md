# Error Handling

## Result / Option `src/native/result.c`/`option.c`

Created by fallible natives (arity `Result`):
```crux
var r: Result[Int] = int("5");
var arr: Array[Int] = [1,2];
var v = match r {
  Ok(val) => { give val; }
  Err(e) => { give 0; }
};
var unwrapped = r.unwrap(); // assert ok IsOk
var maybe: Option[Int] = arr.pop(); // Option
var x = match maybe {
  Some(v) => give v;
  None => give 0;
};
```

- `Result` methods: `unwrap()`, `unwrap_or(default)`, `is_ok()`, `is_err()` `native_registration.c:359` `result_type`.
- `Option`: `unwrap()`, `unwrap_or`, `is_some()`, `is_none()` `native_registration.c:370`.
- `result?` is postfix shorthand for extracting an `Ok` value. Like `unwrap()`, it raises
  a runtime panic for `Err`; it does not return the error from the containing function.
- `?` accepts `Result` only. Use `Option.unwrap()`, `unwrap_or()`, or `match` for an option.
- `panic "msg"` immediate `RUNTIME` (longjmp), `error("msg")` → `Error` object, `assert(cond,"msg")` `core`.

## Error Types `object.h:289`

`SYNTAX` `MATH` `BOUNDS` `TYPE` `ARGUMENT_MISMATCH` `IMPORT` etc (23 variants) via `MAKE_GC_SAFE_ERROR` `object.h:20`.

See [Types](types.md), [Match](match.md).
