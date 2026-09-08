# Error `src/native/error.c` `vm.error_type:350` `object.h:289`

`Error` objects carry `ErrorType` 23 variants `SYNTAX` `MATH` `BOUNDS` `TYPE` `ARGUMENT_MISMATCH` `IMPORT` `IO` etc via `MAKE_GC_SAFE_ERROR(vm,msg,TYPE)` `object.h:20`.

## Methods `Error.`

| Method | Signature | Return |
|--------|-----------|--------|
| `type` | `(Error)` | `String` — `ErrorType` name |
| `message` | `(Error)` | `String` — static message |

```crux
var r = int("not_a_num"); // Err
var msg = match r {
  Ok(v) => { give string(v); }
  Err(e) => { give e.message(); }
};
println(msg); // "Invalid integer"
var e = error("boom");
println(e.type()); // "Runtime"
```

Creation via `error(Any)->Error` core fn + natives returning `Result`. Tests `tests/features/result_option_methods.crux`.

See [Result](result.md), [Option](option.md).
