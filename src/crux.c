#include "crux.h"
#include <string.h>
#include "alloc.h"
#include "garbage_collector.h"
#include "object/object.h"
#include "vm.h"

CRUX_API int crux_get_version_number()
{
	return CRUX_VERSION_NUMBER;
}

/* --- Internal Types --- */

CRUX_API void init_crux_configuration(CruxConfiguration *configuration)
{
	configuration->reallocateFn = NULL;
	configuration->resolveModuleFn = NULL;
	configuration->loadModuleFn = NULL;
	configuration->bindForeignMethodFn = NULL;
	configuration->bindForeignClassFn = NULL;
	configuration->writeFn = NULL;
	configuration->errorFn = NULL;
	configuration->initialHeapSize = 1024 * 1024 * 5; // 5MB
	configuration->minHeapSize = 1024 * 1024; // 1MB
	configuration->heapGrowthPercent = 50;
	configuration->userData = NULL;
	configuration->scriptPath = NULL;
}

CRUX_API CruxVM *crux_vm_new(CruxConfiguration *configuration)
{
	return new_vm(configuration);
}

CRUX_API void crux_vm_free(CruxVM *vm)
{
	free_vm(vm);
}

CRUX_API int crux_vm_get_exit_code(CruxVM *vm)
{
	return vm->exit_code;
}

CRUX_API void crux_collect_garbage(CruxVM *vm)
{
	collect_garbage(vm);
}

static void set_current_module_name(CruxVM *vm, const char *module)
{
	if (vm == NULL || vm->current_module_record == NULL || module == NULL || module[0] == '\0') {
		return;
	}

	ObjectString *module_name = copy_string(vm, module, (uint32_t)strlen(module));
	vm->current_module_record->path = module_name;
	table_set(vm, &vm->module_cache, module_name, OBJECT_VAL(vm->current_module_record));
}

CRUX_API CruxInterpretResult crux_interpret(CruxVM *vm, const char *module, const char *source)
{
	set_current_module_name(vm, module);
	return (CruxInterpretResult)interpret(vm, (char *)source);
}

/* --- Slot API Implementation --- */

CRUX_API void crux_ensure_slots(CruxVM *vm, int numSlots)
{
	if (vm->api_stack_capacity >= numSlots)
		return;

	int oldCapacity = vm->api_stack_capacity;
	vm->api_stack_capacity = numSlots;
	vm->api_stack = Crux_reallocate(vm, vm->api_stack, sizeof(CruxValue) * oldCapacity, sizeof(CruxValue) * numSlots);

	for (int i = oldCapacity; i < numSlots; i++) {
		vm->api_stack[i] = CRUX_NIL_VAL;
	}
}

CRUX_API int crux_get_slot_count(CruxVM *vm)
{
	return vm->api_stack_capacity;
}

CRUX_API CruxType crux_get_slot_type(CruxVM *vm, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return CRUX_TYPE_UNKNOWN;
	CruxValue value = vm->api_stack[slot];

	if (crux_is_bool(value))
		return CRUX_TYPE_BOOL;
	if (crux_is_int(value))
		return CRUX_TYPE_INT;
	if (crux_is_float(value))
		return CRUX_TYPE_FLOAT;
	if (crux_is_nil(value))
		return CRUX_TYPE_NULL;

	if (IS_CRUX_OBJECT(value)) {
		switch (OBJECT_TYPE(value)) {
		case OBJECT_STRING:
			return CRUX_TYPE_STRING;
		case OBJECT_ARRAY:
			return CRUX_TYPE_ARRAY;
		case OBJECT_TABLE:
			return CRUX_TYPE_TABLE;
		// TODO: handle foreign types once implemented
		default:
			return CRUX_TYPE_UNKNOWN;
		}
	}

	return CRUX_TYPE_UNKNOWN;
}

CRUX_API bool crux_get_slot_bool(CruxVM *vm, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return false;
	return crux_as_bool(vm->api_stack[slot]);
}

CRUX_API int32_t crux_get_slot_int(CruxVM *vm, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return 0;
	return crux_as_int(vm->api_stack[slot]);
}

CRUX_API double crux_get_slot_double(CruxVM *vm, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return 0.0;
	return crux_as_float(vm->api_stack[slot]);
}

CRUX_API const char *crux_get_slot_string(CruxVM *vm, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return NULL;
	CruxValue value = vm->api_stack[slot];
	if (!IS_CRUX_STRING(value))
		return NULL;
	return AS_C_STRING(value);
}

CRUX_API void crux_set_slot_bool(CruxVM *vm, int slot, bool value)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return;
	vm->api_stack[slot] = crux_bool_val(value);
}

CRUX_API void crux_set_slot_int(CruxVM *vm, int slot, int32_t value)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return;
	vm->api_stack[slot] = crux_int_val(value);
}

CRUX_API void crux_set_slot_double(CruxVM *vm, int slot, double value)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return;
	vm->api_stack[slot] = crux_float_val(value);
}

CRUX_API void crux_set_slot_string(CruxVM *vm, int slot, const char *text)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return;
	vm->api_stack[slot] = OBJECT_VAL(copy_string(vm, text, (uint32_t)strlen(text)));
}

CRUX_API void crux_set_slot_nil(CruxVM *vm, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return;
	vm->api_stack[slot] = CRUX_NIL_VAL;
}

CRUX_API void crux_set_slot_handle(CruxVM *vm, int slot, CruxHandle *handle)
{
	if (slot < 0 || slot >= vm->api_stack_capacity || handle == NULL)
		return;
	vm->api_stack[slot] = handle->value;
}

/* --- Handle API Implementation --- */

CRUX_API CruxHandle *crux_get_slot_handle(CruxVM *vm, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return NULL;

	CruxValue value = vm->api_stack[slot];

	// Allocate handle using the VM's internal allocator
	CruxHandle *handle = Crux_reallocate(vm, NULL, 0, sizeof(CruxHandle));
	if (handle == NULL)
		return NULL;

	handle->value = value;
	handle->prev = NULL;
	handle->next = vm->handles;

	if (vm->handles != NULL) {
		vm->handles->prev = handle;
	}

	vm->handles = handle;
	return handle;
}

CRUX_API void crux_release_handle(CruxVM *vm, CruxHandle *handle)
{
	if (handle == NULL)
		return;

	if (handle->prev != NULL) {
		handle->prev->next = handle->next;
	} else {
		vm->handles = handle->next;
	}

	if (handle->next != NULL) {
		handle->next->prev = handle->prev;
	}

	// Free handle memory
	Crux_reallocate(vm, handle, sizeof(CruxHandle), 0);
}

CRUX_API void crux_get_variable(CruxVM *vm, const char *module, const char *name, int slot)
{
	if (slot < 0 || slot >= vm->api_stack_capacity)
		return;

	ObjectModuleRecord *mod = NULL;
	if (module == NULL || strlen(module) == 0) {
		mod = vm->current_module_record;
	} else {
		// Find module in cache
		ObjectString *module_name = copy_string(vm, module, (uint32_t)strlen(module));
		CruxValue mod_val;
		if (table_get(&vm->module_cache, module_name, &mod_val)) {
			mod = AS_CRUX_MODULE_RECORD(mod_val);
		}
	}

	if (mod == NULL) {
		vm->api_stack[slot] = CRUX_NIL_VAL;
		return;
	}

	ObjectString *var_name = copy_string(vm, name, (uint32_t)strlen(name));
	CruxValue value;
	if (table_get(&mod->publics, var_name, &value)) {
		vm->api_stack[slot] = value;
	} else {
		vm->api_stack[slot] = CRUX_NIL_VAL;
	}
}

CRUX_API CruxInterpretResult crux_call(CruxVM *vm, int argCount)
{
	if (vm->api_stack_capacity < argCount + 1)
		return CRUX_INTERPRET_RUNTIME_PANIC;

	jmp_buf previous_jump_buffer;
	memcpy(previous_jump_buffer, vm->jump_buffer, sizeof(jmp_buf));

	const int jump_code = setjmp(vm->jump_buffer);
	if (jump_code != INTERPRET_OK) {
		memcpy(vm->jump_buffer, previous_jump_buffer, sizeof(jmp_buf));
		return (CruxInterpretResult)jump_code;
	}

	CruxValue callee = vm->api_stack[0];

	// Setup internal stack for the call
	ObjectModuleRecord *mod = vm->current_module_record;

	// Ensure room on internal stack
	push(mod, callee);
	for (int i = 0; i < argCount; i++) {
		push(mod, vm->api_stack[i + 1]);
	}

	if (!call_value(vm, callee, argCount)) {
		memcpy(vm->jump_buffer, previous_jump_buffer, sizeof(jmp_buf));
		return CRUX_INTERPRET_RUNTIME_PANIC;
	}

	// Run the VM until this frame returns
	InterpretResult result = run(vm, true);

	// Get result from top of stack and put in slot 0
	if (result == INTERPRET_OK) {
		vm->api_stack[0] = pop(mod);
	}

	memcpy(vm->jump_buffer, previous_jump_buffer, sizeof(jmp_buf));
	return (CruxInterpretResult)result;
}
