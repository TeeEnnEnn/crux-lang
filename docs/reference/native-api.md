# Native API `include/crux.h:1` (210 lines, Wren-inspired)

## Version `crux.h:23`
`CRUX_VERSION_MAJOR 0 MINOR 22 PATCH 0 STRING "0.22.0" NUMBER (M*1e6+M*1e3+P)`, `crux_get_version_number()`.

## Types `crux.h:38`
`CruxVM`, `CruxHandle`, `CruxValue uint64_t`, `CruxInterpretResult OK/COMPILE_PANIC/RUNTIME_PANIC/EXIT`, `CruxErrorType COMPILE/RUNTIME/STACK_TRACE`, `CruxType BOOL/INT/FLOAT/STRING/ARRAY/TABLE/NULL/FOREIGN/UNKNOWN`.

## Config `crux.h:91`
```c
typedef struct {
  CruxReallocateFn reallocateFn; CruxResolveModuleFn resolveModuleFn;
  CruxLoadModuleFn loadModuleFn; CruxBindForeignMethodFn bindForeignMethodFn;
  CruxBindForeignClassFn bindForeignClassFn; CruxPrintFn writeFn; CruxErrorFn errorFn;
  size_t initialHeapSize (5MB), minHeapSize (1MB); int heapGrowthPercent 50;
  const char* scriptPath; void* userData;
} CruxConfiguration;
```
Callbacks: `CruxReallocateFn(void*memory,size_t newSize,void*userData)->void*`, `CruxPrintFn(vm,text)`, `CruxErrorFn(vm,CruxErrorType,module,line,message)`, `CruxForeignMethodFn(vm)`, `CruxResolveModuleFn(vm,importer,name)->const char*`, `CruxLoadModuleResult{source,onComplete,userData}`.

## VM `crux.h:91`
`init_crux_configuration(config)`, `crux_vm_new(config)->CruxVM*`, `crux_vm_free`, `crux_vm_get_exit_code`, `crux_collect_garbage`, `crux_interpret(vm,module,source)->CruxInterpretResult`.

## Slots `crux.h:54`
`crux_ensure_slots(vm,n)`, `crux_get_slot_count/type`, `crux_get_variable(vm,module,name,slot)`, `crux_call(vm,argCount)`, getters/setters `crux_get/set_slot_bool/int/double/string/nil/handle`.

## Handles `crux.h:38`
`crux_get_slot_handle(vm,slot)->CruxHandle*`, `crux_release_handle`.

## Values `crux.h:54`
`CRUX_QNAN 0x7ffc...`, `CRUX_NIL_VAL` etc, `crux_int_val`, `crux_float_val`, `crux_is_int` etc inline.
