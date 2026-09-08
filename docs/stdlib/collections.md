# Collections `std/collections/*.crux` `CRUX_STDLIB=std`

```crux
use Stack, Set from "std:collections";
var s = new Stack {data = []};
var set = new Set {data = {}};
```

**Stack `stack.crux:5` `pub struct Stack {data: Array[Any]}`** `impl Stack`:
- `push(Any)->Nil` → `self.data.push(v)`
- `pop()->Option[Any]` via `self.data.pop()`
- `peek()->Option[Any]` `Some(self.data[len-1])` (fixed `0` bug)
- `is_empty()->Bool`, `len()->Int`, `clear()->Nil`

**Set `set.crux:1` `pub struct Set {data: Table[Any,Bool]}`** (`Table[Any,Bool]` trick, `keys()` `Result` via `match keys {Ok(v)=>give v}`):
- `add(Any)->Bool` (`self.data[value]=true` `vm_run.c:689` `IS_CRUX_HASHABLE`)
- `remove(Any)->Bool` (`has_key`+`remove`)
- `contains(Any)->Bool` (`has_key`), `size()->Int` (`len(keys)`), `is_empty`, `clear()->Nil` (`self.data={}`)
- `union/intersect/difference(Set)->Set`, `is_subset/is_disjoint(Set)->Bool`, `to_array()->Array[Any]` (`keys`)

Hashable `NIL/INT/FLOAT/BOOL/STRING` `object.h:101`.

Tests `std/_std_test/collections_test.crux` 121 lines.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
