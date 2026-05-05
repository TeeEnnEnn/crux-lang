# Crux Embedding Example

This example demonstrates how to embed the Crux Virtual Machine into a C application. It covers initialization, custom memory management, I/O redirection, and bi-directional communication between C and Crux.

## Key Features Demonstrated

1.  **Custom Configuration:** Initializing the VM with `CruxConfiguration`.
2.  **Custom Allocator:** Hooking into `reallocateFn` to track or manage memory usage.
3.  **I/O Redirection:** Using `writeFn` to capture script output (e.g., for logging to a GUI console).
4.  **Native Function Binding:** Exposing C functions to Crux using the `native` keyword and `bindForeignMethodFn`.
5.  **Slot API:** Safely passing arguments from C to Crux and retrieving return values.
6.  **Calling Crux from C:** Looking up global functions and executing them using `crux_call`.

## How to Build and Run

To build this example, use CMake from the project root:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Then run the example executable:

```bash
./examples/embedding/crux_embedding_example
```

## Explanation of the Example

The host application (`main.c`):
1.  Defines a C function `native_add`.
2.  Sets up a `CruxConfiguration` with a custom allocator and printer.
3.  Injects a `source` string that declares a `native fn c_add`.
4.  Executes the script.
5.  Finds the Crux function `crux_test` in the module.
6.  Calls that function from C, passing `32`.
7.  Crux then calls back into C's `native_add` to add `10`, returning `42`.
8.  The C host reads the result `42` from the API stack.

## File Structure

*   `main.c`: The host application implementation.
*   `README.md`: This documentation.
