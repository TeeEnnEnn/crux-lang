# Control Flow

## If / While `src/compiler/compiler_statements.c:5`

```crux
if x > 5 { println("big"); } else { println("small"); }
while x < 10 { x += 1; }
```

Narrowing inside `if (typeof(x)=="Int")` `tests/compiler/flow_typing.crux:17`.

## For C-style `compiler_statements.c:30`

```crux
var sum=0;
for var i=0; i<10; i+=1 { sum+=i; }
for ; a<10; a+=1 {} // pre-declared
for ;; { break; } // infinite
```

`break`/`continue` depth `FRAMES_MAX=128` `common.h:16`, `LOOP_WHILE/FOR` `vm.h:125`.

## For-in `compiler_statements.c:30` `for var x in iterable`

```crux
for var item in [1,2,3] { sum+=item; }
for var v in (iter([1,2])?) { println(string(v)); }
```
Iterable (`is_iterable_type` `type_system.c:212`): `Array`/`Range`/`Buffer`/`String`/`Vector`/`Matrix`/`Tuple`/`Iterator`/custom `struct` with `__iter() -> Iterator` + `__next() -> Option[T]` `type_system.c:212` `get_iterable_element_type`. See [Iterators](iterators.md), `tests/features/for_in.crux:5`, `user_iterators.crux:30`.

## Panic / Assert

```crux
panic "boom"; // runtime panic RUNTIME
assert(x==1, "msg"); // core fn len/error/assert
```

 vs `error("msg")` → `Result[Error]` `src/native/error.c`.

## Give `compiler_match.c:212`

`give` inside `match` arm → `OP_GIVE` immediate return value, vs block `=> { }`.

See [Match](match.md), [Iterators](iterators.md).
