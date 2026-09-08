# Match `src/compiler/compiler_match.c:1`

```crux
var m = match 2 {
  2 => give "two";
  default => give "not";
};
var r = match Ok("hi") {
  Ok(v) => { give v; }
  Err(e) => { give e.message(); }
};
var o = match Some(42) {
  Some(v) => give v;
  None => give 0;
};
var u: Int|String = "hello";
var t = match u {
  Int => give "int";
  String => give "string";
};
```

Arms:
- Literal `expr =>`
- Type `Int =>` (union member)
- Result `Ok(v)`/`Err(e)` binding (`_`)
- Option `Some(v)`/`None`
- `default` (mandatory if not exhaustive)

Exhaustiveness (`MatchExhaustiveness` `compiler_match.c:212`):
- `Result` needs `Ok`+`Err` or `default`
- `Option` needs `Some`+`None` or `default`
- Union needs all members or `default`
- Else `default` required.

`give` vs block: `2 => give "two";` vs `2 => { println("two"); }` (last value returned if no `give`). `OP_MATCH` `chunk.h:6` + `MatchHandlerStack` `vm.h:125`.

Tested `tests/features/match.crux:1` (74 lines).

See [Error Handling](error-handling.md).
