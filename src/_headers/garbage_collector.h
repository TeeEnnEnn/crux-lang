#ifndef MEMORY_H
#define MEMORY_H

#include "object/object.h"
#include "slab_allocator.h"

/**
 * Allocates memory for an object of type `type` with `count` elements.
 * On allocation failure, jumps to the vm panic jump buffer
 */
#define ALLOCATE(vm, type, count) (type *)Crux_reallocate(vm, NULL, 0, sizeof(type) * count)

/**
 * Frees memory for an object of type `type` with `count` elements.
 * On allocation failure, jumps to the vm panic jump buffer
 */
#define FREE(vm, type, pointer) Crux_reallocate(vm, pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 2 ? 2 : (capacity) * 2)

/**
 * Grows an array of type `type` with `count` elements.
 * On allocation failure, jumps to the vm panic jump buffer
 */
#define GROW_ARRAY(vm, type, pointer, oldCount, newCount)                                                              \
	(type *)Crux_reallocate(vm, pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(vm, type, pointer, oldCount) Crux_reallocate(vm, pointer, sizeof(type) * (oldCount), 0)

void *allocate_object_with_gc(CruxVM *vm, size_t size);

CruxObject *allocate_pooled_object(CruxVM *vm, size_t size, ObjectType type);

#define ALLOCATE_OBJECT(vm, type, objectType) (type *)allocate_pooled_object(vm, sizeof(type), objectType)

/**
 * @brief Reallocates a block of memory.
 *
 * This function acts as a wrapper around `realloc` and `free`, providing
 * garbage collection integration and debugging features. It updates the CruxVM's
 * `bytesAllocated` counter, potentially triggers garbage collection if
 * allocation exceeds the `nextGC` threshold, and handles allocation failures.
 *
 * @param vm The virtual machine.
 * @param pointer The old block of memory to reallocate. If `NULL`, it's
 * equivalent to `malloc`.
 * @param oldSize The old size of the memory block. If zero, it's equivalent to
 * `malloc`.
 * @param newSize The new size of the memory block. If zero, it's equivalent to
 * `free`.
 *
 * @return A pointer to the reallocated memory block. Returns `NULL` if
 * `newSize` is zero. Exits the program if allocation fails and `realloc`
 * returns `NULL` when `newSize` is not zero.
 */
void *Crux_reallocate(CruxVM *vm, void *pointer, size_t oldSize, size_t newSize);

/**
 * @brief Marks a CruxValue as reachable during garbage collection.
 * @param vm The virtual machine.
 * @param value The CruxValue to mark.
 */
void mark_value(CruxVM *vm, CruxValue value);

/**
 * @brief Performs a full garbage collection cycle.
 *
 * This function orchestrates the garbage collection process:
 * 1. Marks root objects using `markRoots`.
 * 2. Traces references from gray objects using `traceReferences`.
 * 3. Removes white (unmarked) entries from the string interning table.
 * 4. Sweeps unmarked objects and frees their memory using `sweep`.
 * 5. Updates the `nextGC` threshold based on the current allocated memory.
 *
 * @param vm The virtual machine.
 */
void collect_garbage(CruxVM *vm);

/**
 * @brief Frees all remaining objects in the CruxVM's object list.
 *
 * This function is called when the CruxVM is shut down to free all objects that
 * are still allocated. It iterates through the object list and frees each
 * object using `freeObject`. It also frees the gray stack.
 *
 * @param vm The virtual machine.
 */
void free_objects(CruxVM *vm, bool free_all);

void mark_object_internal(CruxVM* vm, CruxObject* object);


/**
 * @brief Marks an object as reachable during garbage collection.
 *
 * This function marks the given `object` as reachable, preventing it from being
 * freed by the garbage collector. If the object is not already marked, it is
 * marked and added to the gray stack for further processing in the mark phase.
 *
 * @param vm The virtual machine.
 * @param object The object to mark. If `NULL`, the function returns
 * immediately.
 */
static inline void mark_object(CruxVM *vm, CruxObject *object)
{
	if (object == NULL || object_is_marked(object) || object_is_immortal(object))
		return;

	mark_object_internal(vm, object);
}



#endif // MEMORY_H
