# Foreign Functions `include/crux.h:77` `examples/embedding/README.md:190`

Crux:
```crux
native fn c_add(a:Int,b:Int)->Int;
pub fn crux_test(x){ return c_add(x,10); }
```
C:
```c
static void native_add(CruxVM* vm){ int a=crux_get_slot_int(vm,1); int b=crux_get_slot_int(vm,2); crux_set_slot_int(vm,0,a+b); }
static CruxForeignMethodFn bind(CruxVM* vm,const char* mod,const char* cls,bool isStatic,const char* sig){
 if(strcmp(sig,"c_add")==0) return native_add; return NULL; }
config.bindForeignMethodFn=bind;
```

Convention: slot0 result, slot1.. args, `void` return, write result to 0. Signature string is Crux declaration name.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
