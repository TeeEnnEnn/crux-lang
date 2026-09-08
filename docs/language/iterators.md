# Iterators `src/type_system/type_system.c:212` `get_iterable_element_type`

```crux
for var x in [1,2,3] { println(string(x)); }
for var c in "hi" { println(c); }
for var i in 0..1..4 {} // Range
for var v in (iter([1,2])?) {}
```

Iterable: `Array`/`Range`/`Buffer`/`String`/`Vector`/`Matrix`/`Tuple`/`Iterator`/custom struct:

```crux
struct Counter { count: Int }
impl Counter {
  fn __iter() -> Iterator[Int] { return new Iterator {iterable=self, index=0}; } // actually via crux runtime
  fn __next() -> Option[Int] { if self.count>3 { return None; } self.count+=1; return Some(self.count); }
}
for var v in myCounter { println(string(v)); } // tests/features/user_iterators.crux:30
```

Protocol: `OP_ITER_INIT` → `new_iterator` `object.h:588`, `OP_ITER_NEXT` → `iterate_next` `object.h:588` (`iter`/`next` core fns `native_registration.c:258` `len`/`iter`/`next`).

`is_collection_type`/`is_iterable_type` `type_system.c:212`.

See [Control Flow](control-flow.md), [Slices](slices.md).
