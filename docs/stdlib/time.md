# Time `src/native/time.c` module `time` `native_registration.c:675`

Fns `use ... from "crux:time"`:

| Fn | Signature | Return |
|----|-----------|--------|
| `sleep_s/sleep_ms` | `(Numeric)` | `Nil` |
| `time_s/time_ms` | `()` | `Float` — `gettimeofday` `object.h` |
| `year/month/day/hour/minute/second/weekday/day_of_year` | `()` | `Int` — `localtime` |

```crux
use sleep_ms, time_ms from "crux:time";
var t0 = time_ms();
sleep_ms(100);
println(string(time_ms() - t0)); // ~100
```

Tests `tests/modules/time.crux`.

See [Sys](sys.md) `platform`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
