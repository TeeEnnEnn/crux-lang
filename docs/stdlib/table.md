# Table `src/native/tables.c` `ObjectTable` `object.h:437` `vm.table_type:336`

Literal `{"a":1,"b":2}` `OP_TABLE` `chunk.h:6`, `Table[K,V]` `K` must be `hashable` `HASHABLE_TYPE` `value.h:47` `NIL|INT|FLOAT|BOOL|STRING` `common.h:12` `is_valid_table_key_type` `type_system.c:212`.

Access `t["key"]` `OP_GET_COLLECTION` `vm_run.c:594` (checks `IS_CRUX_HASHABLE` `object.h:101`), `t["key"]=val` `OP_SET_COLLECTION` `vm_run.c:689` (fixed `IS_CRUX_HASHABLE`).

## Methods `Table[K,V].` 7 `native_registration.c:336`

| Method | Signature | Return |
|--------|-----------|--------|
| `values` | `(Table)` | `Result[Array[Any]]` |
| `keys` | `(Table)` | `Result[Array[Any]]` |
| `pairs` | `(Table)` | `Result[Array[Array]]` |
| `remove` | `(Table,Hashable)` | `Result[Any]` |
| `get` | `(Table,Hashable)` | `Result[Any]` |
| `has_key` | `(Table,Hashable)` | `Bool` |
| `get_or_else` | `(Table,Hashable,Any)` | `Any` |

```crux
var t = {"a":1,"b":2};
println(string(t.has_key("a"))); // true
var v = t.get("a")?; // 1
var keys = t.keys()?; // ["a","b"]
for var k in (t.keys()? ) { } // need unwrap via match in Set.crux:40
t.remove("a")?;
println(string(len(t.keys()?))); // 1
```

Tests `tests/modules/table.crux` + `std/collections/set.crux:1` `Table[Any,Bool]` backing.

See [Collections](collections.md) `Set`.
