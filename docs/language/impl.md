# Impl — Methods `src/compiler/compiler_declarations.c:58`

```crux
struct Point { x: Int, y: Int }
impl Point {
  fn x_squared() -> Int { return self.x * self.x; }
  fn y_squared() -> Int { return self.y * self.y; }
  static fn create(x:Int,y:Int) -> Point { return new Point {x=x,y=y}; }
}
var p = new Point {x=10,y=20};
p.x_squared(); // 100
Point::create(1,2); // static via ::
```

- `impl Struct { fn ... static fn ... }` `src/compiler/compiler_core.c:54`.
- Instance `fn` has implicit `self` (`self.x`) → `OP_INVOKE` `src/_headers/chunk.h:6`.
- Static `static fn` → `OP_STATIC_INVOKE` + `::` `CRUX_TOKEN_COLON_COLON` `src/scanner.c:584`, called as `Struct::method()` `tests/features/structs.crux:38`.
- `ObjectStruct` `object.h:339` stores `methods` + `static_methods` `Table`.

Limits: `impl` only once per struct, no inheritance.

See [Structs](structs.md), [Functions](functions.md).

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
