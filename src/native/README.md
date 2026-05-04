# Native Modules, Functions and Methods

Native Modules, functions and methods are C functions that are exposed to all Crux scripts. 

These functions can either be:
1. standard functions
2. methods on native types


## Conventions

Return types:

All native functions are derived from this prototype
```c
typedef CruxValue (*CruxCallable)(CruxVM *vm, const CruxValue *args);
```

So all native functions must return CruxValue. 

By convention functions that may fail must return a Result. 

### What may cause a function to fail

In the past, failure may have been caused by many reason including memory allocation failure. 

However, because setjmp/longjmp are now used for memory allocation failures failure can only be caused by logic errors within the  native function. 

So a function that allocates does not automatically become a failing function.

When returning from a function that fails use the following macros: 

```c
#define MAKE_GC_SAFE_ERROR(vm, gc_safe_static_message, gc_safe_error_type)

#define MAKE_GC_SAFE_RESULT(vm, object)
```

### Arguments

For methods the first argument is the object upon which the method acts. 

```
// For methods
args[0] -> The object the method acts on
```

For functions and methods (after the 0th argument) the arguments are in the order in which the arguments are defined.


## Registration

Native functions, and methods are registered in the `src/native/native_registration.c` file.

In this file all of the functions/methods are added to their respective arrays with their argument types and arities defined.

This registration allows the functions/methods to be used from Crux scripts.
