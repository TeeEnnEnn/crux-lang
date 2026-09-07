# Type System `src/type_system/type_system.c:322` `type_table.c` `value.h:47` `object.h:362`

## Tables `type_table.c` `ObjectTypeTable`

`TYPE_NAME_BUF_SIZE 256` `common.h:26` `initialTypeTable 16` `sprint_type_to`.

## Masks `value.h:47` `TypeMask` 24 bits

`NIL 1<<0` `BOOL 1<<1` `INT 1<<2` `FLOAT 1<<3` `STRING 1<<4` `ARRAY 1<<5` `TABLE 1<<6` `ERROR 1<<7` `RESULT 1<<8` `RANDOM 1<<9` `FILE 1<<10` `STRUCT 1<<11` `VECTOR 1<<12` `COMPLEX 1<<13` `MATRIX 1<<14` `TUPLE 1<<15` `BUFFER 1<<16` `RANGE 1<<17` `ITERATOR 1<<18` `OPTION 1<<19` + `NUMERIC=INT|FLOAT` `HASHABLE=NIL|INT|FLOAT|BOOL|STRING` `NEVER 1<<30` `ANY 1<<31`.

## Equality `types_equal` `type_system.c:322`

Structural except `Struct` nominal `ObjectStruct.name` `object.h:339` identity.

## Compatibility `types_compatible` `type_system.c:412`

- `Any`↔any, `Never`→any
- `Int→Float` promotion (`expected Float got Int` true)
- `Vector[3]→Vector[]` `-1` wildcard, `Matrix[2,3]→Matrix[,]`
- `Union` subset `A|B` order independent `UNI` `native_registration.c:54` vs expected `C|D`
- `shape {x:Int}` structural field subset `type_system.c:412` vs `struct` nominal
- `Table[K,V]` both params, `Array[T]` `element_type`, `Result[Ok]` `ok_type`, `Option[Some]`, function `(A,B)->Ret` contravariant `arg_types`
- Helpers `strip_type` `is_numeric_type` `is_collection_type` `is_iterable_type` `get_iterable_element_type` (`Iterator/Array/Range/Buffer/String/Vector/Matrix/Tuple/Struct __iter`) `type_system.c:212`.

## Narrowing `current_narrowing` `compiler_statements.c:if_statement`

`if (typeof(x)=="Int")` `flow_typing.crux:17` restores after `else`; `if (maybe!=nil)` strips `Nil`.

See [Language Types](../language/types.md) + [Reference Type Masks](../reference/type-masks.md).
