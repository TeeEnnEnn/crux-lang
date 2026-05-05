# Embedding Crux

This example shows how to use Crux as an embedded scripting language inside a C
application. The host application owns the process, creates a `CruxVM`, provides
callbacks for I/O and native functions, executes Crux source, and can call public
Crux functions through the slot API.

The current embedding API is intentionally close to Wren's model: the host
configures the VM, values move through numbered slots, Crux can call C functions
declared with `native fn`, and C can look up and call public Crux functions.

## Build And Run

From the repository root:

```bash
cmake --build build
```

Run the example:

```bash
./build/crux_embedding_example
```

On Windows:

```powershell
.\build\crux_embedding_example.exe
```

The example should print `C: Result from Crux was 42`.

## Files

- `main.c`: Host application embedding Crux.
- `README.md`: This guide.

## Embedding Model

The host program uses Crux in four steps:

1. Configure the VM with `CruxConfiguration`.
2. Create the VM with `crux_vm_new()`.
3. Execute Crux source with `crux_interpret()`.
4. Exchange values with Crux through slots and `crux_call()`.

Minimal lifecycle:

```c
CruxConfiguration config;
init_crux_configuration(&config);

CruxVM* vm = crux_vm_new(&config);

CruxInterpretResult result = crux_interpret(vm, "main", source);

crux_vm_free(vm);
```

The `module` argument to `crux_interpret()` names the source module. Use the same
name later when looking up public variables or functions.

## Configuration

Always initialize the configuration before setting fields:

```c
CruxConfiguration config;
init_crux_configuration(&config);
```

Important fields:

- `writeFn`: receives text printed by Crux.
- `errorFn`: receives compile/runtime errors and stack trace entries.
- `reallocateFn`: optional host allocator hook.
- `bindForeignMethodFn`: maps Crux `native fn` declarations to C functions.
- `scriptPath`: optional path/name used by the CLI-style host for the main script.
- `userData`: arbitrary host pointer available to callbacks through the VM config.

Callbacks are optional, but real hosts should usually provide at least `writeFn`
and `errorFn` so output and failures can be routed into logs, consoles, editors,
or GUI widgets.

## Output And Errors

Crux output is delivered through `writeFn`:

```c
static void host_write(CruxVM* vm, const char* text) {
  printf("[crux] %s", text);
}

config.writeFn = host_write;
```

Errors are delivered through `errorFn`:

```c
static void host_error(
    CruxVM* vm,
    CruxErrorType type,
    const char* module,
    int line,
    const char* message) {
  fprintf(stderr, "%s:%d: %s\n", module, line, message);
}

config.errorFn = host_error;
```

`CruxErrorType` distinguishes compile errors, runtime errors, and stack trace
entries. Treat stack trace callbacks as additional context for a runtime error.

## Slots

The public API moves values through numbered slots. Before using slots, reserve
enough of them:

```c
crux_ensure_slots(vm, 2);
```

Slots are used for both directions:

- C puts arguments into slots before calling Crux.
- C reads return values from slots after the call.
- Native C functions called by Crux read their arguments from slots and write
  their result to slot `0`.

Common setters:

```c
crux_set_slot_int(vm, 1, 32);
crux_set_slot_double(vm, 1, 3.14);
crux_set_slot_bool(vm, 1, true);
crux_set_slot_string(vm, 1, "hello");
crux_set_slot_nil(vm, 1);
```

Common getters:

```c
int32_t i = crux_get_slot_int(vm, 0);
double d = crux_get_slot_double(vm, 0);
bool b = crux_get_slot_bool(vm, 0);
const char* s = crux_get_slot_string(vm, 0);
```

Use `crux_get_slot_type()` when the host needs to validate a slot before reading
it.

## Calling Crux From C

Only public Crux globals are exposed through `crux_get_variable()`.

Crux source:

```crux
pub fn add_ten(x) {
  return x + 10;
}
```

C host:

```c
crux_interpret(vm, "example", source);

crux_ensure_slots(vm, 2);
crux_get_variable(vm, "example", "add_ten", 0);
crux_set_slot_int(vm, 1, 32);

CruxInterpretResult result = crux_call(vm, 1);
if (result == CRUX_INTERPRET_OK) {
  int32_t value = crux_get_slot_int(vm, 0);
}
```

Calling convention:

- Slot `0` contains the callable.
- Slots `1..argCount` contain arguments.
- After a successful call, slot `0` contains the return value.

If a runtime error occurs during `crux_call()`, the function returns a
`CruxInterpretResult` error instead of crashing the host.

## Calling C From Crux

Crux declares host-provided functions with `native fn`:

```crux
native fn c_add(a: Int, b: Int) -> Int;

pub fn crux_test(x) {
  return c_add(x, 10);
}
```

The host provides a C function:

```c
static void native_add(CruxVM* vm) {
  int32_t a = crux_get_slot_int(vm, 1);
  int32_t b = crux_get_slot_int(vm, 2);
  crux_set_slot_int(vm, 0, a + b);
}
```

Then it binds the Crux declaration to that C function:

```c
static CruxForeignMethodFn bind_foreign_method(
    CruxVM* vm,
    const char* module,
    const char* className,
    bool isStatic,
    const char* signature) {
  if (strcmp(module, "example") == 0 && strcmp(signature, "c_add") == 0) {
    return native_add;
  }

  return NULL;
}

config.bindForeignMethodFn = bind_foreign_method;
```

Native C function convention:

- Slot `0` is reserved for the result.
- Arguments start at slot `1`.
- The function returns `void`.
- The C function must write its return value into slot `0`.

## Handles

Handles let the host keep a Crux value alive outside the immediate slot window:

```c
CruxHandle* handle = crux_get_slot_handle(vm, 0);
```

Later, the host can put the value back into a slot:

```c
crux_set_slot_handle(vm, 0, handle);
```

Release the handle when it is no longer needed:

```c
crux_release_handle(vm, handle);
```

Use handles for long-lived callbacks or values stored in host data structures.
Do not store raw `CruxValue` objects long-term unless they are protected by a
handle or rooted another way.

## Garbage Collection

The VM manages Crux objects automatically. API slots and active handles are
marked as roots, so values stored there are protected from collection.

The host may request a collection:

```c
crux_collect_garbage(vm);
```

The custom allocator hook can be used to track allocation volume or route memory
through host-specific allocation systems.

## Native Modules Today

The current API supports native functions compiled into the host application.
That is enough to build embedded modules such as:

- math or utility extensions
- game-engine functions
- SDL/raylib-style procedural bindings
- application automation hooks
- test helpers

For example, a host can expose a small raylib-like surface:

```crux
native fn init_window(width: Int, height: Int, title: String) -> Nil;
native fn window_should_close() -> Bool;
native fn begin_drawing() -> Nil;
native fn draw_text(text: String, x: Int, y: Int, size: Int) -> Nil;
native fn end_drawing() -> Nil;
```

The host links raylib, binds those names to C functions, and Crux scripts call
them normally.

## Current Limitations

The embedding API is usable, but it is not yet a complete native package ABI.

Currently missing or early:

- no dynamic shared-library loader for `crux:module` native packages
- no public foreign object API for wrapping C pointers/resources
- no finalizer API for host-owned resources exposed as Crux objects
- no polished callback/event API for GUI or game-loop integrations
- native function type metadata is not yet fully preserved for host-bound
  functions

This means SDL/raylib-style function bindings are practical today, while richer
object-heavy integrations such as GTK widgets should wait for proper foreign
object support.

## Integration Checklist

When embedding Crux into another project:

1. Link against `crux_lib`.
2. Include `crux.h`.
3. Initialize `CruxConfiguration`.
4. Provide `writeFn` and `errorFn`.
5. Provide `bindForeignMethodFn` if Crux scripts declare `native fn`.
6. Create the VM with `crux_vm_new()`.
7. Load/produce Crux source text.
8. Execute it with `crux_interpret(vm, moduleName, source)`.
9. Look up public entry points with `crux_get_variable()`.
10. Pass arguments through slots and call with `crux_call()`.
11. Release handles and free the VM during shutdown.

The example in `main.c` is the smallest end-to-end version of that flow.
