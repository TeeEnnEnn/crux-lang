# Structs & Shapes

## Struct (nominal) `src/compiler/compiler_declarations.c:58`

```crux
struct Point { x: Int, y: Int }
var p = new Point { x=10, y=20 };
assert(p.x == 10);
pub struct Player { name: String, score: Int|Float, hp: Float }
var pl = new Player { name="Hero", score=1500, hp=100.0 };
```

- `new Struct { field=expr }` → `OP_STRUCT_INSTANCE` `src/_headers/chunk.h:6`.
- Fields typed `field:Type` else `Any`. `types_equal` checks `Struct.name` identity (nominal) `type_system.c:322`.
- `pub struct` top-level.

## Shape (structural) `src/compiler/compiler_core.c:96`

```crux
var s: shape {x:Int} = new Point {x=1,y=2}; // ok: Point has x
var sh: shape {a:Int,b:String} = {"a":1,"b":"hi"}; // also shape via Table? no, use struct
```

`shape {field:Type}` structural subset check `is_compatible` `type_system.c:412`.

## Struct vs Shape

- `struct` equality is name-identity, `shape` is field-subset.
- `shape` useful for gradual duck-typing.

See [Impl](impl.md), [Types](types.md).
