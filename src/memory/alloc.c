#include <stdio.h>
#include <stdlib.h>

#include "alloc.h"
#include "crux.h"
#include "garbage_collector.h"
#include "object.h"
#include "panic.h"
#include "slab_allocator.h"
#include "vm.h"

void *alloc_memory(CruxVM *vm, size_t size)
{
	if (size == 0)
		return NULL;

	if (size <= 24)
		return allocate_from_slab(vm->slab_24);
	if (size <= 32)
		return allocate_from_slab(vm->slab_32);
	if (size <= 48)
		return allocate_from_slab(vm->slab_48);
	if (size <= 64)
		return allocate_from_slab(vm->slab_64);

	return malloc(size);
}

void free_memory(CruxVM *vm, void *ptr, const size_t size)
{
	if (!ptr || size == 0) {
		vm_error(vm, Crux_ERROR_RUNTIME, 0, "Error: NULL pointer given for memory to free.\n");
		return;
	}

	vm->bytes_allocated -= size;

	if (size <= 24) {
		free_from_slab(vm->slab_24, ptr);
	} else if (size <= 32) {
		free_from_slab(vm->slab_32, ptr);
	} else if (size <= 48) {
		free_from_slab(vm->slab_48, ptr);
	} else if (size <= 64) {
		free_from_slab(vm->slab_64, ptr);
	} else {
		free(ptr);
	}
}

void *allocate_object_with_gc(CruxVM *vm, const size_t size)
{
	vm->bytes_allocated += size;
	#ifdef DEBUG_STRESS_GC
	collect_garbage(vm);
	#else
	if (vm->bytes_allocated > vm->next_gc) {
		collect_garbage(vm);
	}
	#endif

	void *result = alloc_memory(vm, size);
	if (result == NULL) {
		collect_garbage(vm);
		result = alloc_memory(vm, size);
		if (result == NULL) {
			if (vm->current_module_record) {
				runtime_panic(vm->current_module_record, MEMORY, "Failed to allocate %zu bytes.", size);
			} else {
				vm_error(vm, Crux_ERROR_RUNTIME, 0, "Fatal error - Out of Memory: Failed to allocate %zu bytes.\n", size);

				longjmp(vm->jump_buffer, INTERPRET_RUNTIME_ERROR);
			}
		}
	}
	return result;
}

void *Crux_reallocate(CruxVM *vm, void *pointer, const size_t oldSize, const size_t newSize)
{
	vm->bytes_allocated += newSize - oldSize;
	if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
		collect_garbage(vm);
#else
		if (vm->bytes_allocated > vm->next_gc) {
			collect_garbage(vm);
		}
#endif
	}

	if (newSize == 0) {
		free(pointer);
		return NULL;
	}

	void *result = realloc(pointer, newSize);
	if (result == NULL) {
		collect_garbage(vm);
		result = realloc(pointer, newSize);
		if (result == NULL) {
			if (oldSize > 0) free(pointer);

			if (vm->current_module_record) {
				runtime_panic(vm->current_module_record, MEMORY, "Failed to reallocate %zu bytes.", newSize);
			} else {
				vm_error(vm, Crux_ERROR_RUNTIME, 0, "Fatal error - Out of Memory: Failed to reallocate %zu bytes.\n", newSize);
				longjmp(vm->jump_buffer, INTERPRET_RUNTIME_ERROR);
			}
		}
	}

	return result;
}
