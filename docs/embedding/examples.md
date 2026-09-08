# Examples — Link `examples/embedding/README.md:1` 333 lines + `examples/embedding/main.c:45`

See the full [Embedding Crux](../../examples/embedding/README.md) guide.

## Minimal `examples/embedding/main.c:45`

```c
CruxConfiguration config; init_crux_configuration(&config);
config.writeFn=host_write; config.errorFn=host_error;
config.bindForeignMethodFn=bind_foreign_method;
CruxVM* vm=crux_vm_new(&config);
const char* source = "native fn c_add(a:Int,b:Int)->Int; pub fn crux_test(x){return c_add(x,10);}";
crux_interpret(vm,"example",source);
crux_ensure_slots(vm,2); crux_get_variable(vm,"example","crux_test",0);
crux_set_slot_int(vm,1,32); if(crux_call(vm,1)==CRUX_INTERPRET_OK){ int v=crux_get_slot_int(vm,0); printf("C: %d\n",v); } // 42
crux_vm_free(vm);
```

Build `cmake --build build` → `./build/crux_embedding_example` `src/README.md:4`.

## Raylib Sketch `embedding/README.md:287`

```crux
native fn init_window(width:Int,height:Int,title:String)->Nil;
native fn window_should_close()->Bool;
native fn begin_drawing()->Nil;
native fn draw_text(text:String,x:Int,y:Int,size:Int)->Nil;
native fn end_drawing()->Nil;
```

Host links `raylib`, binds via `bindForeignMethodFn`, Crux scripts call normally. Procedural bindings practical today; object-heavy (GTK widgets) needs future `CruxForeignClassMethods` `crux.h:91` (`allocate/finalize`).

See [Lifecycle](lifecycle.md) 11 steps.
