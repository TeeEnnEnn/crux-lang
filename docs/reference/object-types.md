# Object Types `src/_headers/object/object.h:104` `SENTINEL_OBJECT_COUNT 32` `static_assert`

```c
typedef enum {
 OBJECT_STRING=0, FUNCTION, NATIVE_CALLABLE, CLOSURE, UPVALUE,
 ARRAY, TABLE, ERROR, RESULT, RANDOM, FILE, MODULE_RECORD,
 STRUCT, STRUCT_INSTANCE, VECTOR, COMPLEX, MATRIX, BUFFER,
 TUPLE, RANGE, ITERATOR, TYPE_RECORD, TYPE_TABLE, OPTION, COROUTINE
} ObjectType;
```

| Type | Struct `object.h:244` | Use |
|------|----------------------|-----|
| `STRING` | `ObjectString {chars,byte_length,code_point_length,hash}` `utf8.h` | `String` |
| `FUNCTION/CLOSURE/UPVALUE` | `ObjectFunction {arity,upvalue_count,Chunk,module_record}` `ObjectClosure {function,upvalues}` | `fn` |
| `ARRAY/TABLE` | `ObjectArray {values,size,capacity}` `ObjectTable {entries,capacity,size}` | `Array`/`Table` |
| `ERROR/RESULT/OPTION` | `ObjectError {message,type,is_panic}` `ObjectResult {is_ok,as}` | `Error`/`Result`/`Option` |
| `RANDOM/FILE` | `ObjectRandom {seed}` `ObjectFile {path,mode,FILE*,is_open}` | `Random`/`File` |
| `MODULE_RECORD` | `ObjectModuleRecord {path,global_names,publics,types,stack}` `object.h:500` | import |
| `STRUCT/STRUCT_INSTANCE` | `ObjectStruct {name,fields,methods,static_methods}` `ObjectStructInstance {struct_type,fields}` | `struct` |
| `VECTOR/COMPLEX/MATRIX` | `ObjectVector {dimensions,s_components[4]/h_components}` `ObjectComplex {real,imag}` `ObjectMatrix {row_dim,col_dim,data}` | `Vector[3]` |
| `BUFFER/TUPLE/RANGE/ITERATOR` | `ObjectBuffer {data,read_pos,write_pos}` `ObjectTuple {elements}` `ObjectRange {start,end,step}` `ObjectIterator {iterable,index}` | `Buffer` etc |
| `TYPE_RECORD/TYPE_TABLE` | `ObjectTypeRecord {base_type,union as}` `ObjectTypeTable {entries}` | `TypeMask` |

Tagged `CRUX_TAGGED_OBJECT` `object.h:144`: `tagged_next` 48-bit pointer `0x0000FFFFFFFFFFFF`, type `5-bit 0x1F <<48`, marked `53`, immortal `54`; else `struct {next,type,is_marked,is_immortal}`. `UINTPTR_MAX==0xFFFFFFFF` error on 32-bit `object.h:136`.

Helpers `is_object_type` `object_get_type` `object.h:544`.

See [Type Masks](type-masks.md), [Opcodes](opcodes.md).
