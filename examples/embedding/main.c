#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crux.h"

// 1. Custom Allocator (Tracking allocations)
static int total_allocations = 0;
void *my_allocator(void *ptr, size_t newSize, void *userData)
{
	if (newSize == 0) {
		free(ptr);
		return NULL;
	}
	total_allocations++;
	return realloc(ptr, newSize);
}

// 2. Custom Output (Capturing prints)
void my_printer(CruxVM *vm, const char *text)
{
	printf("[Crux Output] %s", text);
}

// 3. Native Function (C function called from Crux)
// Signature: native fn c_add(a: Int, b: Int) -> Int;
void native_add(CruxVM *vm)
{
	// Arguments are in slots 1 and 2 (slot 0 is receiver/function)
	int32_t a = crux_get_slot_int(vm, 1);
	int32_t b = crux_get_slot_int(vm, 2);

	// Set result in slot 0
	crux_set_slot_int(vm, 0, a + b);
}

// 4. Binding Logic
CruxForeignMethodFn my_binder(CruxVM *vm, const char *module, const char *className, bool isStatic,
							  const char *signature)
{
	if (strcmp(signature, "c_add") == 0)
		return native_add;
	return NULL;
}

int main()
{
	printf("--- Starting Crux Embedding Example ---\n");

	// Initialize Configuration
	CruxConfiguration config;
	init_crux_configuration(&config);

	config.reallocateFn = my_allocator;
	config.writeFn = my_printer;
	config.bindForeignMethodFn = my_binder;

	// Create VM
	CruxVM *vm = crux_vm_new(&config);
	if (!vm)
		return 1;

	// Source code with native declaration
	const char *source = "native fn c_add(a: Int, b: Int) -> Int;\n"
						 "pub fn crux_test(x) {\n"
						 "  println(\"Crux: adding \" + string(x) + \" and 10 via C...\");\n"
						 "  return c_add(x, 10);\n"
						 "}\n"
						 "println(\"Crux: Script initialized.\");\n";

	// Interpret script
	printf("Interpreting script...\n");
	crux_interpret(vm, "example", source);

	// Call Crux function from C
	printf("\nCalling 'crux_test(32)' from C...\n");
	crux_ensure_slots(vm, 2);
	crux_get_variable(vm, "example", "crux_test", 0); // Get function into slot 0
	crux_set_slot_int(vm, 1, 32); // Set argument in slot 1

	CruxInterpretResult res = crux_call(vm, 1); // Call with 1 argument

	if (res == CRUX_INTERPRET_OK) {
		int32_t result = crux_get_slot_int(vm, 0);
		printf("C: Result from Crux was %d\n", result);

		if (result != 42) {
			fprintf(stderr, "Test Failed: Expected 42, got %d\n", result);
			return 1;
		}
	} else {
		printf("C: Call failed!\n");
		return 1;
	}

	printf("\nTotal host-side allocations: %d\n", total_allocations);

	crux_vm_free(vm);
	printf("--- Embedding Example Finished Successfully ---\n");
	return 0;
}
