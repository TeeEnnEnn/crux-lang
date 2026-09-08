# Types — Full `TypeMask` `src/_headers/value.h:47` + `src/type_system/type_system.c:322`

## Masks `value.h:47`

| Mask | Bits | Crux |
|------|------|------|
| `NIL_TYPE` `BOOL_TYPE` `INT_TYPE` `FLOAT_TYPE` `STRING_TYPE` | 1<<0..4 | primitives |
| `ARRAY_TYPE` `TABLE_TYPE` `ERROR_TYPE` `RESULT_TYPE` `RANDOM_TYPE` `FILE_TYPE` `STRUCT_TYPE` `VECTOR_TYPE` `COMPLEX_TYPE` `MATRIX_TYPE` `TUPLE_TYPE` `BUFFER_TYPE` `RANGE_TYPE` `ITERATOR_TYPE` `OPTION_TYPE` | 1<<5.. |
| `NUMERIC_TYPE` `INT\|FLOAT` | union | |
| `HASHABLE_TYPE` `NIL\|INT\|FLOAT\|BOOL\|STRING` | for `Table[K]` `value.h:322` `is_valid_table_key_type` | |
| `ANY_TYPE` `1<<31`, `NEVER_TYPE` `1<<30` | `Any` escapes, `Never` bottom (`panic`/`return`/`break`) | |

## Generic Syntax `compiler_core.c:23` `parse_type_record`

```
Type :=
  "Nil"|"Bool"|"Int"|"Float"|"String"|"Any"|"Never"
| "Array" "[" Type "]"
| "Table" "[" Type "," Type "]"   // K must be hashable
| "Vector" "[" Int "]"            // -1 = wildcard Vector[]
| "Matrix" "[" Int "," Int "]"    // Matrix[,]
| "Tuple" "[" Type, ... "]"
| "Result" "[" Type "]"  "Option" "[" Type "]"
| "shape" "{" field ":" Type, ... "}"
| "(" Type, ... ")" "->" Type      // function
| Type "|" Type                    // union order-independent
| identifier                       // struct name or type alias
```

Examples `tests/compiler/basic_types.crux:24`:
```crux
var a: Array[Int] = [1,2];
var t: Table[String,Int] = {"a":1};
var v: Vector[3] = Vector(3,[1.0,2.0,3.0])?;
var m: Matrix[2,3] = Matrix(2,3)?;
var tup: Tuple[Int,String] = Tuple([1,"a"]);
var r: Result[Int] = Ok(5);
var o: Option[Int] = Some(5);
var f: (Int,String)->Bool = fn (a:Int,b:String)->Bool { return true; };
var u: Int|Float|String = 5; // union
var s: shape {x:Int,y:Int} = new Point{x=1,y=2}; // structural
```

## Compatibility `types_compatible` `type_system.c:412`

- `Any` ↔ any, `Never` → any.
- `Int` ↔ `Float` (`expected Float got Int` true).
- `Vector[3]` → `Vector[]`, `Matrix[2,3]` → `Matrix[,]`.
- `shape {x:Int}` compatible if fields ⊆ target (structural), `struct Point` only if same `ObjectStruct.name` (nominal).
- `Union` intimately: `Int|String` compatible with `String|Int` (order independent), subset checks.

## Flow Narrowing `compiler_statements.c:if_statement`

```crux
var x: Int|String = 5;
if typeof(x) == "Int" {
  var y: Int = x + 10; // narrowed
}
var maybe: Int|Nil = nil;
if maybe != nil {
  var v: Int = maybe + 1; // stripped Nil
}
```

See [Type Masks Reference](../reference/type-masks.md), [Structs](structs.md).
