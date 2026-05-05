#include <stdio.h>
#include <stdlib.h>

#include "alloc.h"
#include "crux.h"
#include "garbage_collector.h"
#include "object.h"
#include "panic.h"
#include "slab_allocator.h"
#include "vm.h"

/**
 * @brief Internal gateway for all memory operations.
 * Respects the host-provided allocator if available.
 */
static void* internal_reallocate(CruxVM* vm, void* ptr, size_t old_size, size_t new_size) {
    if (vm && vm->config.reallocateFn) {
        return vm->config.reallocateFn(ptr, new_size, vm->config.userData);
    }
    
    // Default implementation
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, new_size);
}

void *alloc_memory(CruxVM *vm, size_t size)
{
	if (size == 0)
		return NULL;

    // Use slab for small object optimizations
    if (vm) {
        if (size <= 24) return allocate_from_slab(vm->slab_24);
        if (size <= 32) return allocate_from_slab(vm->slab_32);
        if (size <= 48) return allocate_from_slab(vm->slab_48);
        if (size <= 64) return allocate_from_slab(vm->slab_64);
    }

	return internal_reallocate(vm, NULL, 0, size);
}

void free_memory(CruxVM *vm, void *ptr, const size_t size)
{
	if (!ptr || size == 0) {
		return;
	}

    if (vm) {
	    vm->bytes_allocated -= size;
        if (size <= 24) {
            free_from_slab(vm->slab_24, ptr);
            return;
        } else if (size <= 32) {
            free_from_slab(vm->slab_32, ptr);
            return;
        } else if (size <= 48) {
            free_from_slab(vm->slab_48, ptr);
            return;
        } else if (size <= 64) {
            free_from_slab(vm->slab_64, ptr);
            return;
        }
    }

    internal_reallocate(vm, ptr, size, 0);
}

void *allocate_object_with_gc(CruxVM *vm, const size_t size)
{
    if (vm) {
	    vm->bytes_allocated += size;
	    #ifdef DEBUG_STRESS_GC
	    collect_garbage(vm);
	    #else
	    if (vm->bytes_allocated > vm->next_gc) {
		    collect_garbage(vm);
	    }
	    #endif
    }

	void *result = alloc_memory(vm, size);
	if (result == NULL && size > 0) {
		collect_garbage(vm);
		result = alloc_memory(vm, size);
		if (result == NULL) {
			if (vm && vm->current_module_record) {
				runtime_panic(vm->current_module_record, MEMORY, "Failed to allocate %zu bytes.", size);
			} else {
                if (vm) {
				    vm_error(vm, Crux_ERROR_RUNTIME, 0, "Fatal error - Out of Memory: Failed to allocate %zu bytes.\n", size);
				    longjmp(vm->jump_buffer, INTERPRET_RUNTIME_ERROR);
                } else {
                    fprintf(stderr, "Fatal error - Out of Memory: Failed to allocate %zu bytes.\n", size);
                    exit(1);
                }
			}
		}
	}
	return result;
}

void *Crux_reallocate(CruxVM *vm, void *pointer, const size_t oldSize, const size_t newSize)
{
    if (vm) {
	    vm->bytes_allocated += (newSize - oldSize);
	    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
		    collect_garbage(vm);
#else
		    if (vm->bytes_allocated > vm->next_gc) {
			    collect_garbage(vm);
		    }
#endif
	    }
    }

	void *result = internal_reallocate(vm, pointer, oldSize, newSize);
	if (result == NULL && newSize > 0) {
		collect_garbage(vm);
		result = internal_reallocate(vm, pointer, oldSize, newSize);
		if (result == NULL) {
			if (vm && vm->current_module_record) {
				runtime_panic(vm->current_module_record, MEMORY, "Failed to reallocate %zu bytes.", newSize);
			} else {
                if (vm) {
				    vm_error(vm, Crux_ERROR_RUNTIME, 0, "Fatal error - Out of Memory: Failed to reallocate %zu bytes.\n", newSize);
				    longjmp(vm->jump_buffer, INTERPRET_RUNTIME_ERROR);
                } else {
                     fprintf(stderr, "Fatal error - Out of Memory: Failed to reallocate %zu bytes.\n", newSize);
                     exit(1);
                }
			}
		}
	}

	return result;
}
