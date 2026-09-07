# Slices & Ranges `src/native/range.c` `vm_run.c:689` `chunk.h:6`

```crux
var arr = [1,2,3,4];
var sl = arr[1..3]; // [2,3] via Range
var r = Range(0,10,1)?; // or 0..10 / 0..1..10
for var x in r { println(string(x)); }
var s = "hello";
println(s[1..3]); // "el" `string.get` + `OP_GET_SLICE`
```

- `Range` `ObjectRange` `object.h:523` `start/end/step` `new_range` `validate_range_values` `object.h:588` (`step !=0`).
- Methods `Range.` 7 `native_registration.c:534`: `contains(Range,Int)->Bool`, `to_array()->Result[Array[Int]]`, `start/end/step->Int`, `is_empty->Bool`, `reversed->Range`.
- Slicing `OP_GET_SLICE` `chunk.h:6` `range_indices_in_bounds` `vm_helpers.c:774` bounds `0<=start<end<=len`.
- `arr[0]` `OP_GET_COLLECTION` `vm_run.c:594` `IS_CRUX_HASHABLE` `object.h:101`, `arr[0]=val` `OP_SET_COLLECTION` `vm_run.c:689`.

```crux
var nums=[0,1,2,3,4];
var a = nums[1..4]; // [1,2,3]
var b = nums[2..3]; // [2]
var c = Range(0,5,2)?; // 0,2,4
println(string(c.to_array()?));
```

Tested `tests/features/collection_slicing.crux` `collection_literals.crux` + `tests/modules/range.crux`.

See [Iterators](iterators.md), [Control Flow](control-flow.md), and
[Array](../stdlib/array.md).
