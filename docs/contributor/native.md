# Native `src/native/README.md` `src/native/native_registration.c:252`

Add new module:
1. `src/native/my.c` `CruxCallable` `CruxValue my_fn(CruxVM* vm,const CruxValue* args)` `object.h:419` return `CruxValue`, fail → `MAKE_GC_SAFE_ERROR(vm,"msg",TYPE)` `object.h:20` else `MAKE_GC_SAFE_RESULT(vm,obj)`.
2. `src/_headers/native/my.h` header.
3. Register `native_registration.c:252` `initialize_std_lib`:
   ```c
   #define t_my REC(MY_TYPE)
   Callable fns[]={{"my_fn",my_fn,1,ARGS(t_my),t_int}};
   init_module(vm,"my",fns,ARRAY_COUNT(fns));
   // methods
   Callable methods[]={{"do",my_method,2,ARGS(t_my,hashable),res_nil}};
   init_type_method_table(vm,&vm->my_type,methods,ARRAY_COUNT(methods));
   ```
   Use `ARGS/ARGS0`, `NAMES`, `RES/ARR/TBL/OPT/UNI/FUNC` `native_registration.c:54`.
4. Limit `NATIVE_FUNCTION_MAX_ARGS=8` `object.h:15`, immortal `object_set_immortal`.

140 registrations `array 16`, `buffer 29` etc see `stdlib/README.md`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
