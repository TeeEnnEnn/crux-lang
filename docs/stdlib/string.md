# String `src/native/string.c` `ObjectString` `object.h:244` `vm.string_type:278`

Strings provide 28 registered methods:

| Method | Group | Signature | Return |
|--------|-------|-----------|--------|
| `byte_length` | size | `(String)` | `Int` |
| `first/last` | access | `(String)` | `String` |
| `get` |  | `(String,Int)` | `Result[String]` |
| `to_upper/to_lower` | case | `(String)` | `String` |
| `is_upper/is_lower/is_alpha/is_digit/is_space/is_alphanum/is_empty` | check | `(String)` | `Bool` |
| `strip` | trim | `(String)` | `Result[String]` |
| `substring` | slice | `(String,Int,Int)` | `Result[String]` |
| `split` | split | `(String,String)` | `Result[Array[String]]` |
| `contains` | search | `(String,String)` | `Bool` |
| `starts_with/ends_with` |  | `(String,String)` | `Result[Bool]` |
| `concat` |  | `(String,String)` | `Result[String]` |
| `reverse` |  | `(String)` | `Result[String]` |
| `find` |  | `(String,String)` | `Int` |
| `repeat` |  | `(String,Int)` | `Result[String]` |
| `join` |  | `(String,Array[Any])` | `String` |
| `pad_left/pad_right` | pad | `(String,Int,String)` | `String` |
| `count` |  | `(String,String)` | `Int` |
| `replace` |  | `(String,String,String)` | `Result[String]` |

Also `len(Any)->Int` core, `string(Any)->String`.

```crux
var s = "hello";
println(s.to_upper()); // "HELLO"
println(string(s.byte_length())); // 5
var parts = "a,b".split(",")?; // ["a","b"]
println(s.substring(1,3)?); // "ell"
```

Iterable `for var c in "hi"` `type_system.c:212`.

Tests `tests/modules/string.crux`.
