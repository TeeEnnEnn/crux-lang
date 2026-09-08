# Statistics `std/statistics/*.crux` `CRUX_STDLIB=std`

```crux
use mean, median from "std:statistics";
```

## `mean(Array[Int|Float])->Float` `statistics.crux:3`

```crux
pub fn mean(data: Array[Int|Float]) -> Float {
  var count = len(data); if count==0 { return 0.0; }
  var sum = data.reduce(fn (acc,curr){return acc+curr;},0.0)? as Float;
  return sum / count;
}
```

`reduce` returns a `Result`; `?` extracts its value and panics if reduction fails. The
result is then cast to `Float`.

## `median(Array[Int|Float])->Float` `statistics.crux:12`

```crux
pub fn median(data: Array[Int|Float]) -> Float {
  if len(data)==0 { return 0.0; }
  var sorted = data.sort()?; // Result[Array]
  if count%2==0 { return (sorted[int(count/2-1)?]+sorted[int(count/2)?])/2.0; }
  else { return sorted[int(count/2)?]; }
}
```

Empty → `0.0`.

```crux
var data=[345,242,981.53,431.3,456];
println(string(mean(data))); // 491.166
println(string(median(data))); // 431.3
```

Tests `std/_std_test/stats_test.crux` + `tests/benchmarks/*` not stdlib.

See [Array](array.md) `sort`/`reduce`.
