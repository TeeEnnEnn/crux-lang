#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "alloc.h"
#include "common.h"
#include "compiler/compiler_core.h"
#include "garbage_collector.h"
#include "object.h"
#include "panic.h"
#include "slab_allocator.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

static uint64_t gc_now_ns(void)
{
#ifdef _WIN32
	LARGE_INTEGER frequency;
	LARGE_INTEGER counter;

	// Get the number of ticks per second
	QueryPerformanceFrequency(&frequency);
	// Get the current tick count
	QueryPerformanceCounter(&counter);

	// Convert to nanoseconds: (ticks * 1,000,000,000) / frequency
	return (counter.QuadPart * 1000000000LL) / frequency.QuadPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

static size_t slab_pool_capacity(const SlabAllocator *allocator)
{
	if (allocator == NULL) {
		return 0;
	}

	size_t slab_count = 0;
	for (const SlabNode *node = allocator->slab_head; node != NULL; node = node->next) {
		slab_count++;
	}

	return slab_count * allocator->capacity;
}

static size_t table_tombstone_count(const Table *table)
{
	size_t tombstones = 0;
	if (!table || !table->entries)
		return 0;

	for (int i = 0; i < table->capacity; i++) {
		const Entry *entry = &table->entries[i];
		if (entry->key == NULL && !IS_NIL(entry->value)) {
			tombstones++;
		}
	}
	return tombstones;
}

static size_t compute_next_gc_threshold(const CruxVM *vm)
{
	const size_t growth_target = (size_t)((double)vm->bytes_allocated * vm->heap_growth_factor);
	const size_t delta_target = vm->bytes_allocated + vm->min_gc_growth_delta;

	size_t next_gc = growth_target > delta_target ? growth_target : delta_target;
	if (next_gc < vm->min_gc_heap_size) {
		next_gc = vm->min_gc_heap_size;
	}
	return next_gc;
}

void mark_object_internal(CruxVM *vm, CruxObject *object)
{
	object_set_marked(object, true);

	if (vm->gray_capacity < vm->gray_count + 1) {
		vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
		CruxObject **new_objects = realloc(vm->gray_stack, vm->gray_capacity * sizeof(CruxObject *));
		if (new_objects == NULL) {
			if (vm->current_module_record)
				runtime_panic(vm->current_module_record, MEMORY, "Failed to grow gray stack.");
			else
				longjmp(vm->jump_buffer, 1);
		}
		vm->gray_stack = new_objects;
	}

	vm->gray_stack[vm->gray_count++] = object;

	if ((uint32_t)vm->gray_count > vm->gc_last_gray_peak)
		vm->gc_last_gray_peak = (uint32_t)vm->gray_count;
	if ((uint32_t)vm->gray_count > vm->gc_max_gray_peak)
		vm->gc_max_gray_peak = (uint32_t)vm->gray_count;
}

void mark_value(CruxVM *vm, const CruxValue value)
{
	if (IS_CRUX_OBJECT(value)) {
		mark_object(vm, AS_CRUX_OBJECT(value));
	}
}

void mark_array(CruxVM *vm, const ValueArray *array)
{
	for (int i = 0; i < array->count; i++) {
		mark_value(vm, array->values[i]);
	}
}

void mark_object_array(CruxVM *vm, const CruxValue *values, const uint32_t size)
{
	for (uint32_t i = 0; i < size; i++) {
		mark_value(vm, values[i]);
	}
}

void mark_object_table(CruxVM *vm, const ObjectTableEntry *entries, const uint32_t capacity)
{
	if (!entries)
		return;
	for (uint32_t i = 0; i < capacity; i++) {
		if (entries[i].is_occupied) {
			mark_value(vm, entries[i].value);
			mark_value(vm, entries[i].key);
		}
	}
}

void mark_type_table(CruxVM *vm, ObjectTypeTable *table)
{
	if (!table)
		return;
	mark_object(vm, (CruxObject *)table);
	for (int i = 0; i < table->capacity; i++) {
		if (table->entries[i].key != NULL) {
			mark_object(vm, (CruxObject *)table->entries[i].value);
		}
	}
}

void mark_type_record(CruxVM *vm, ObjectTypeRecord *rec)
{
	if (!rec)
		return;
	mark_object(vm, (CruxObject *)rec);
}

static void mark_object_struct(CruxVM *vm, ObjectStruct *structure)
{
	mark_object(vm, (CruxObject *)structure->name);
	mark_table(vm, &structure->fields);
	mark_table(vm, &structure->methods);
	mark_object(vm, (CruxObject *)structure);
}

static void mark_struct_instance(CruxVM *vm, ObjectStructInstance *instance)
{
	for (int i = 0; i < instance->field_count; i++) {
		mark_value(vm, instance->fields[i]);
	}
	mark_object_struct(vm, instance->struct_type);
	mark_object(vm, (CruxObject *)instance);
}

typedef void (*BlackenFunction)(CruxVM *vm, CruxObject *object);
typedef void (*FreeFunction)(CruxVM *vm, CruxObject *object);

static void blacken_closure(CruxVM *vm, CruxObject *object);
static void blacken_function(CruxVM *vm, CruxObject *object);
static void blacken_upvalue(CruxVM *vm, CruxObject *object);
static void blacken_array(CruxVM *vm, CruxObject *object);
static void blacken_table(CruxVM *vm, CruxObject *object);
static void blacken_error(CruxVM *vm, CruxObject *object);
static void blacken_native_callable(CruxVM *vm, CruxObject *object);
static void blacken_result(CruxVM *vm, CruxObject *object);
static void blacken_random(CruxVM *vm, CruxObject *object);
static void blacken_file(CruxVM *vm, CruxObject *object);
static void blacken_module_record(CruxVM *vm, CruxObject *object);
static void blacken_struct(CruxVM *vm, CruxObject *object);
static void blacken_struct_instance(CruxVM *vm, CruxObject *object);
static void blacken_vector(CruxVM *vm, CruxObject *object);
static void blacken_complex(CruxVM *vm, CruxObject *object);
static void blacken_string(CruxVM *vm, CruxObject *object);
static void blacken_range(CruxVM *vm, CruxObject *object);
static void blacken_iterator(CruxVM *vm, CruxObject *object);
static void blacken_set(CruxVM *vm, CruxObject *object);
static void blacken_buffer(CruxVM *vm, CruxObject *object);
static void blacken_tuple(CruxVM *vm, CruxObject *object);
static void blacken_matrix(CruxVM *vm, CruxObject *object);
static void blacken_type_record(CruxVM *vm, CruxObject *object);
static void blacken_type_table(CruxVM *vm, CruxObject *object);
static void blacken_option(CruxVM *vm, CruxObject *object);

static const BlackenFunction blacken_dispatch[] = {
	[OBJECT_STRING] = blacken_string,
	[OBJECT_FUNCTION] = blacken_function,
	[OBJECT_NATIVE_CALLABLE] = blacken_native_callable,
	[OBJECT_CLOSURE] = blacken_closure,
	[OBJECT_UPVALUE] = blacken_upvalue,
	[OBJECT_ARRAY] = blacken_array,
	[OBJECT_TABLE] = blacken_table,
	[OBJECT_ERROR] = blacken_error,
	[OBJECT_RESULT] = blacken_result,
	[OBJECT_OPTION] = blacken_option,
	[OBJECT_RANDOM] = blacken_random,
	[OBJECT_FILE] = blacken_file,
	[OBJECT_MODULE_RECORD] = blacken_module_record,
	[OBJECT_STRUCT] = blacken_struct,
	[OBJECT_STRUCT_INSTANCE] = blacken_struct_instance,
	[OBJECT_VECTOR] = blacken_vector,
	[OBJECT_COMPLEX] = blacken_complex,
	[OBJECT_RANGE] = blacken_range,
	[OBJECT_ITERATOR] = blacken_iterator,
	[OBJECT_BUFFER] = blacken_buffer,
	[OBJECT_TUPLE] = blacken_tuple,
	[OBJECT_MATRIX] = blacken_matrix,
	[OBJECT_TYPE_RECORD] = blacken_type_record,
	[OBJECT_TYPE_TABLE] = blacken_type_table,
};

static void blacken_object(CruxVM *vm, CruxObject *object)
{
#ifdef DEBUG_LOG_GC
	vm_print(vm, "%p blacken ", (void *)object);
	print_value(OBJECT_VAL(object), false);
	vm_print(vm, "\n");
#endif

	const ObjectType type = object_get_type(object);
	if (type < (ObjectType)(sizeof(blacken_dispatch) / sizeof(blacken_dispatch[0]))) {
		blacken_dispatch[type](vm, object);
	}
}

static void blacken_closure(CruxVM *vm, CruxObject *object)
{
	const ObjectClosure *closure = (ObjectClosure *)object;
	mark_object(vm, (CruxObject *)closure->function);
	for (int i = 0; i < closure->upvalue_count; i++) {
		mark_object(vm, (CruxObject *)closure->upvalues[i]);
	}
}

static void blacken_function(CruxVM *vm, CruxObject *object)
{
	const ObjectFunction *function = (ObjectFunction *)object;
	mark_object(vm, (CruxObject *)function->name);
	mark_object(vm, (CruxObject *)function->module_record);
	mark_array(vm, &function->chunk.constants);
}

static void blacken_upvalue(CruxVM *vm, CruxObject *object)
{
	mark_value(vm, ((ObjectUpvalue *)object)->closed);
}

static void blacken_array(CruxVM *vm, CruxObject *object)
{
	const ObjectArray *array = (ObjectArray *)object;
	mark_object_array(vm, array->values, array->size);
}

static void blacken_table(CruxVM *vm, CruxObject *object)
{
	const ObjectTable *table = (ObjectTable *)object;
	mark_object_table(vm, table->entries, table->capacity);
}

static void blacken_error(CruxVM *vm, CruxObject *object)
{
	const ObjectError *error = (ObjectError *)object;
	mark_object(vm, (CruxObject *)error->message);
}

static void blacken_native_callable(CruxVM *vm, CruxObject *object)
{
	const ObjectNativeCallable *native = (ObjectNativeCallable *)object;
	mark_object(vm, (CruxObject *)native->name);
	if (native->arg_types) {
		for (int i = 0; i < native->arity; i++) {
			mark_type_record(vm, native->arg_types[i]);
		}
	}
	mark_type_record(vm, native->return_type);
}

static void blacken_iterator(CruxVM *vm, CruxObject *object)
{
	const ObjectIterator *iterator = (ObjectIterator *)object;
	mark_value(vm, iterator->iterable);
}

static void blacken_result(CruxVM *vm, CruxObject *object)
{
	const ObjectResult *result = (ObjectResult *)object;
	if (result->is_ok) {
		mark_value(vm, result->as.value);
	} else {
		mark_object(vm, (CruxObject *)result->as.error);
	}
}

static void blacken_option(CruxVM *vm, CruxObject *object)
{
	const ObjectOption *option = (ObjectOption *)object;
	mark_value(vm, option->value);
}

static void blacken_random(CruxVM *vm, CruxObject *object)
{
	(void)vm;
	(void)object;
}

static void blacken_file(CruxVM *vm, CruxObject *object)
{
	const ObjectFile *file = (ObjectFile *)object;
	mark_object(vm, (CruxObject *)file->path);
	mark_object(vm, (CruxObject *)file->mode);
}

static void blacken_module_record(CruxVM *vm, CruxObject *object)
{
	const ObjectModuleRecord *module = (ObjectModuleRecord *)object;
	mark_object(vm, (CruxObject *)module->path);
	mark_table(vm, &module->global_names);
	mark_table(vm, &module->publics);
	mark_type_table(vm, module->types);
	mark_object(vm, (CruxObject *)module->module_closure);
	mark_object(vm, (CruxObject *)module->enclosing_module);
	for (uint32_t i = 0; i < module->global_count; i++) {
		mark_value(vm, module->globals[i]);
	}

	for (const CruxValue *slot = module->stack; slot < module->stack_top; slot++) {
		mark_value(vm, *slot);
	}
	for (int i = 0; i < module->frame_count; i++) {
		mark_object(vm, (CruxObject *)module->frames[i].closure);
	}
	for (ObjectUpvalue *upvalue = module->open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
		mark_object(vm, (CruxObject *)upvalue);
	}
}

static void blacken_struct(CruxVM *vm, CruxObject *object)
{
	ObjectStruct *structure = (ObjectStruct *)object;
	mark_object_struct(vm, structure);
}

static void blacken_struct_instance(CruxVM *vm, CruxObject *object)
{
	ObjectStructInstance *instance = (ObjectStructInstance *)object;
	mark_struct_instance(vm, instance);
}

static void blacken_vector(CruxVM *vm, CruxObject *object)
{
	(void)vm;
	(void)object;
}

static void blacken_complex(CruxVM *vm, CruxObject *object)
{
	(void)vm;
	(void)object;
}

static void blacken_matrix(CruxVM *vm, CruxObject *object)
{
	(void)vm;
	(void)object;
}

static void blacken_string(CruxVM *vm, CruxObject *object)
{
	(void)vm;
	(void)object;
}

static void blacken_range(CruxVM *vm, CruxObject *object)
{
	(void)vm;
	(void)object;
}

static void blacken_buffer(CruxVM *vm, CruxObject *object)
{
	(void)vm;
	(void)object;
}

static void blacken_tuple(CruxVM *vm, CruxObject *object)
{
	const ObjectTuple *tuple = (ObjectTuple *)object;
	mark_object_array(vm, tuple->elements, tuple->size);
}

static void blacken_type_table(CruxVM *vm, CruxObject *object)
{
	const ObjectTypeTable *table = (ObjectTypeTable *)object;
	if (!table->entries)
		return;
	for (int i = 0; i < table->capacity; i++) {
		const TypeEntry *entry = &table->entries[i];
		if (entry->key == NULL)
			continue;
		mark_object(vm, (CruxObject *)entry->key);
		mark_type_record(vm, entry->value);
	}
}

static void blacken_type_record(CruxVM *vm, CruxObject *object)
{
	const ObjectTypeRecord *rec = (ObjectTypeRecord *)object;
	switch (rec->base_type) {
	case ARRAY_TYPE:
		mark_type_record(vm, rec->as.array_type.element_type);
		break;
	case ITERATOR_TYPE:
		mark_type_record(vm, rec->as.iterator_type.element_type);
		break;
	case TABLE_TYPE:
		mark_type_record(vm, rec->as.table_type.key_type);
		mark_type_record(vm, rec->as.table_type.value_type);
		break;
	case RESULT_TYPE:
		mark_type_record(vm, rec->as.result_type.ok_type);
		break;
	case OPTION_TYPE:
		mark_type_record(vm, rec->as.option_type.some_type);
		break;
	case STRUCT_TYPE:
		if (rec->as.struct_type.definition) {
			mark_object(vm, (CruxObject *)rec->as.struct_type.definition);
		}
		mark_type_table(vm, rec->as.struct_type.field_types);
		break;
	case FUNCTION_TYPE:
		if (rec->as.function_type.arg_types) {
			for (int i = 0; i < rec->as.function_type.arg_count; i++) {
				mark_type_record(vm, rec->as.function_type.arg_types[i]);
			}
		}
		mark_type_record(vm, rec->as.function_type.return_type);
		break;
	case TUPLE_TYPE: {
		for (int i = 0; i < rec->as.tuple_type.element_count; i++) {
			mark_type_record(vm, rec->as.tuple_type.element_types[i]);
		}
		break;
	}
	case UNION_TYPE:
		if (rec->as.union_type.element_types) {
			for (int i = 0; i < rec->as.union_type.element_count; i++) {
				mark_type_record(vm, rec->as.union_type.element_types[i]);
			}
		}
		if (rec->as.union_type.element_names) {
			for (int i = 0; i < rec->as.union_type.element_count; i++) {
				mark_object(vm, (CruxObject *)rec->as.union_type.element_names[i]);
			}
		}
		break;
	case SHAPE_TYPE:
		mark_type_table(vm, rec->as.shape_type.element_types);
		break;
	default:
		break;
	}
}

static void free_object_string(CruxVM *vm, CruxObject *object);
static void free_object_function(CruxVM *vm, CruxObject *object);
static void free_object_native_callable(CruxVM *vm, CruxObject *object);
static void free_object_closure(CruxVM *vm, CruxObject *object);
static void free_object_upvalue(CruxVM *vm, CruxObject *object);
static void free_object_array(CruxVM *vm, CruxObject *object);
static void free_object_table_wrapper(CruxVM *vm, CruxObject *object);
static void free_object_error(CruxVM *vm, CruxObject *object);
static void free_object_result(CruxVM *vm, CruxObject *object);
static void free_object_option(CruxVM *vm, CruxObject *object);
static void free_object_random(CruxVM *vm, CruxObject *object);
static void free_object_file(CruxVM *vm, CruxObject *object);
static void free_object_module_record_wrapper(CruxVM *vm, CruxObject *object);
static void free_object_struct(CruxVM *vm, CruxObject *object);
static void free_object_struct_instance(CruxVM *vm, CruxObject *object);
static void free_object_vector(CruxVM *vm, CruxObject *object);
static void free_object_complex(CruxVM *vm, CruxObject *object);
static void free_object_set(CruxVM *vm, CruxObject *object);
static void free_object_range(CruxVM *vm, CruxObject *object);
static void free_object_iterator(CruxVM *vm, CruxObject *object);
static void free_object_buffer(CruxVM *vm, CruxObject *object);
static void free_object_tuple(CruxVM *vm, CruxObject *object);
static void free_object_matrix(CruxVM *vm, CruxObject *object);
static void free_object_type_record(CruxVM *vm, CruxObject *object);
static void free_object_type_table(CruxVM *vm, CruxObject *object);

static const FreeFunction free_dispatch[] = {
	[OBJECT_STRING] = free_object_string,
	[OBJECT_FUNCTION] = free_object_function,
	[OBJECT_NATIVE_CALLABLE] = free_object_native_callable,
	[OBJECT_CLOSURE] = free_object_closure,
	[OBJECT_UPVALUE] = free_object_upvalue,
	[OBJECT_ARRAY] = free_object_array,
	[OBJECT_TABLE] = free_object_table_wrapper,
	[OBJECT_ERROR] = free_object_error,
	[OBJECT_RESULT] = free_object_result,
	[OBJECT_OPTION] = free_object_option,
	[OBJECT_RANDOM] = free_object_random,
	[OBJECT_FILE] = free_object_file,
	[OBJECT_MODULE_RECORD] = free_object_module_record_wrapper,
	[OBJECT_STRUCT] = free_object_struct,
	[OBJECT_STRUCT_INSTANCE] = free_object_struct_instance,
	[OBJECT_VECTOR] = free_object_vector,
	[OBJECT_COMPLEX] = free_object_complex,
	[OBJECT_RANGE] = free_object_range,
	[OBJECT_ITERATOR] = free_object_iterator,
	[OBJECT_BUFFER] = free_object_buffer,
	[OBJECT_TUPLE] = free_object_tuple,
	[OBJECT_MATRIX] = free_object_matrix,
	[OBJECT_TYPE_RECORD] = free_object_type_record,
	[OBJECT_TYPE_TABLE] = free_object_type_table,
};

static void free_object_string(CruxVM *vm, CruxObject *object)
{
	const ObjectString *string = (ObjectString *)object;
	FREE_ARRAY(vm, char, string->chars, string->byte_length + 1);
	FREE_OBJECT(vm, ObjectString, object);
}

static void free_object_function(CruxVM *vm, CruxObject *object)
{
	ObjectFunction *function = (ObjectFunction *)object;
	free_chunk(vm, &function->chunk);
	FREE_OBJECT(vm, ObjectFunction, object);
}

static void free_object_native_callable(CruxVM *vm, CruxObject *object)
{
	ObjectNativeCallable *native = (ObjectNativeCallable *)object;
	if (native->arg_types) {
		FREE_ARRAY(vm, ObjectTypeRecord *, native->arg_types, native->arity);
	}
	FREE_OBJECT(vm, ObjectNativeCallable, object);
}

static void free_object_closure(CruxVM *vm, CruxObject *object)
{
	const ObjectClosure *closure = (ObjectClosure *)object;
	FREE_ARRAY(vm, ObjectUpvalue *, closure->upvalues, closure->upvalue_count);
	FREE_OBJECT(vm, ObjectClosure, object);
}

static void free_object_upvalue(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectUpvalue, object);
}

static void free_object_array(CruxVM *vm, CruxObject *object)
{
	const ObjectArray *array = (ObjectArray *)object;
	FREE_ARRAY(vm, CruxValue, array->values, array->capacity);
	FREE_OBJECT(vm, ObjectArray, object);
}

static void free_object_table_wrapper(CruxVM *vm, CruxObject *object)
{
	ObjectTable *table = (ObjectTable *)object;
	free_object_table(vm, table);
	FREE_OBJECT(vm, ObjectTable, object);
}

static void free_object_error(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectError, object);
}

static void free_object_result(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectResult, object);
}

static void free_object_option(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectOption, object);
}

static void free_object_random(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectRandom, object);
}

static void free_object_file(CruxVM *vm, CruxObject *object)
{
	const ObjectFile *file = (ObjectFile *)object;
	if (file->file != NULL) {
		fclose(file->file);
	}
	FREE_OBJECT(vm, ObjectFile, object);
}

static void free_object_module_record_wrapper(CruxVM *vm, CruxObject *object)
{
	ObjectModuleRecord *moduleRecord = (ObjectModuleRecord *)object;
	free_object_module_record(vm, moduleRecord);
	FREE_OBJECT(vm, ObjectModuleRecord, object);
}

static void free_object_struct(CruxVM *vm, CruxObject *object)
{
	ObjectStruct *structure = (ObjectStruct *)object;
	free_table(vm, &structure->fields);
	free_table(vm, &structure->methods);
	FREE_OBJECT(vm, ObjectStruct, object);
}

static void free_object_struct_instance(CruxVM *vm, CruxObject *object)
{
	const ObjectStructInstance *instance = (ObjectStructInstance *)object;
	FREE_ARRAY(vm, CruxValue, instance->fields, instance->field_count);
	FREE_OBJECT(vm, ObjectStructInstance, object);
}

static void free_object_vector(CruxVM *vm, CruxObject *object)
{
	const ObjectVector *vector = (ObjectVector *)object;
	if (vector->dimensions > 4) {
		FREE_ARRAY(vm, double, vector->as.h_components, vector->dimensions);
	}
	FREE_OBJECT(vm, ObjectVector, object);
}

static void free_object_complex(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectComplex, object);
}

static void free_object_range(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectRange, object);
}

static void free_object_iterator(CruxVM *vm, CruxObject *object)
{
	FREE_OBJECT(vm, ObjectIterator, object);
}

static void free_object_buffer(CruxVM *vm, CruxObject *object)
{
	const ObjectBuffer *buffer = (ObjectBuffer *)object;
	FREE_ARRAY(vm, uint8_t, buffer->data, buffer->capacity);
	FREE_OBJECT(vm, ObjectBuffer, object);
}

static void free_object_tuple(CruxVM *vm, CruxObject *object)
{
	const ObjectTuple *tuple = (ObjectTuple *)object;
	FREE_ARRAY(vm, CruxValue, tuple->elements, tuple->size);
	FREE_OBJECT(vm, ObjectTuple, object);
}

static void free_object_matrix(CruxVM *vm, CruxObject *object)
{
	const ObjectMatrix *matrix = (ObjectMatrix *)object;
	FREE_ARRAY(vm, double, matrix->data, (uint32_t)matrix->row_dim * matrix->col_dim);
	FREE_OBJECT(vm, ObjectMatrix, object);
}

static void free_object_type_record(CruxVM *vm, CruxObject *object)
{
	ObjectTypeRecord *rec = (ObjectTypeRecord *)object;
	if (rec->base_type == FUNCTION_TYPE) {
		if (rec->as.function_type.arg_types) {
			FREE_ARRAY(vm, ObjectTypeRecord *, rec->as.function_type.arg_types, rec->as.function_type.arg_count);
		}
	} else if (rec->base_type == UNION_TYPE) {
		if (rec->as.union_type.element_types) {
			FREE_ARRAY(vm, ObjectTypeRecord *, rec->as.union_type.element_types, rec->as.union_type.element_count);
		}
		if (rec->as.union_type.element_names) {
			FREE_ARRAY(vm, ObjectString *, rec->as.union_type.element_names, rec->as.union_type.element_count);
		}
	} else if (rec->base_type == TUPLE_TYPE) {
		if (rec->as.tuple_type.element_types && rec->as.tuple_type.element_count >= 0) {
			FREE_ARRAY(vm, ObjectTypeRecord *, rec->as.tuple_type.element_types, rec->as.tuple_type.element_count);
		}
	}
	FREE_OBJECT(vm, ObjectTypeRecord, object);
}

static void free_object_type_table(CruxVM *vm, CruxObject *object)
{
	ObjectTypeTable *table = (ObjectTypeTable *)object;
	if (table->entries) {
		FREE_ARRAY(vm, TypeEntry, table->entries, table->capacity);
	}
	table->capacity = -1;
	table->count = -1;
	table->entries = NULL;
	FREE_OBJECT(vm, ObjectTypeTable, object);
}

void mark_module_roots(CruxVM *vm, ObjectModuleRecord *moduleRecord)
{
	if (moduleRecord->enclosing_module != NULL) {
		mark_module_roots(vm, moduleRecord->enclosing_module);
	}

	mark_object(vm, (CruxObject *)moduleRecord->path);
	mark_table(vm, &moduleRecord->global_names);
	mark_table(vm, &moduleRecord->publics);
	mark_type_table(vm, moduleRecord->types);
	mark_object(vm, (CruxObject *)moduleRecord->module_closure);
	mark_object(vm, (CruxObject *)moduleRecord->enclosing_module);
	for (uint32_t i = 0; i < moduleRecord->global_count; i++) {
		mark_value(vm, moduleRecord->globals[i]);
	}

	for (const CruxValue *slot = moduleRecord->stack; slot < moduleRecord->stack_top; slot++) {
		mark_value(vm, *slot);
	}

	for (int i = 0; i < moduleRecord->frame_count; i++) {
		mark_object(vm, (CruxObject *)moduleRecord->frames[i].closure);
	}

	for (ObjectUpvalue *upvalue = moduleRecord->open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
		mark_object(vm, (CruxObject *)upvalue);
	}

	mark_object(vm, (CruxObject *)moduleRecord);
}

void mark_struct_instance_stack(CruxVM *vm, const StructInstanceStack *stack)
{
	if (stack->structs != NULL) {
		for (uint32_t i = 0; i < stack->count; i++) {
			mark_struct_instance(vm, stack->structs[i]);
		}
	}
}

void mark_roots(CruxVM *vm)
{
	if (vm->current_module_record) {
		mark_module_roots(vm, vm->current_module_record);
	}

	for (uint32_t i = 0; i < vm->import_stack.count; i++) {
		mark_object(vm, (CruxObject *)vm->import_stack.paths[i]);
	}

	mark_table(vm, &vm->module_cache);
	mark_table(vm, &vm->strings);

	// No need to mark type method / function tables or native modules because they only contain immortal objects that
	// will not be collected

	mark_struct_instance_stack(vm, &vm->struct_instance_stack);

	if (vm->main_compiler) {
		mark_compiler_roots(vm, vm->main_compiler);
	}

	for (uint32_t i = 0; i < vm->match_handler_stack.count; i++) {
		mark_value(vm, vm->match_handler_stack.handlers[i].match_bind);
		mark_value(vm, vm->match_handler_stack.handlers[i].match_target);
	}
}

static void trace_references(CruxVM *vm)
{
	while (vm->gray_count > 0) {
		CruxObject *object = vm->gray_stack[--vm->gray_count];
		blacken_object(vm, object);
	}
}

static void free_object(CruxVM *vm, CruxObject *object, bool free_all)
{
#ifdef DEBUG_LOG_GC
	vm_print(vm, "%p free type %d\n", (void *)object, object_get_type(object));
#endif
	if (object == NULL || (object_is_immortal(object) && !free_all))
		return;

	if (object_get_type(object) < (ObjectType)(sizeof(free_dispatch) / sizeof(free_dispatch[0]))) {
		free_dispatch[object_get_type(object)](vm, object);
	}
}

static void sweep(CruxVM *vm)
{
	size_t slots_scanned = 0;
	CruxObject *prev = NULL;
	CruxObject *current = vm->objects;

	while (current != NULL) {
		slots_scanned++;
		CruxObject *next = object_get_next(current);

		if (!object_is_marked(current)) {
			// Unlink dead object
			if (prev == NULL)
				vm->objects = next;
			else
				object_set_next(prev, next);

			free_object(vm, current, false);
			vm->object_count--;
		} else {
			// Unmark and advance
			object_set_marked(current, false);
			prev = current;
		}
		current = next;
	}

	vm->gc_last_sweep_slots_scanned = slots_scanned;
	if (vm->gc_last_sweep_slots_scanned > vm->gc_last_objects_before_sweep) {
		vm->gc_last_sweep_slots_scanned = vm->gc_last_objects_before_sweep;
	}
	vm->gc_sweep_slots_scanned += slots_scanned;
}

void free_objects(CruxVM *vm, bool free_all)
{
	CruxObject *object = vm->objects;
	while (object != NULL) {
		CruxObject *next = object_get_next(object);
		free_object(vm, object, free_all);
		object = next;
	}
	free(vm->gray_stack);
	vm->objects = NULL;
	vm->object_count = 0;
}

void collect_garbage(CruxVM *vm)
{
	if (vm->gc_status == PAUSED)
		return;

	const uint64_t gc_start_ns = gc_now_ns();
	uint64_t phase_start_ns = gc_start_ns;
	vm->gc_last_gray_peak = 0;
	vm->gc_last_bytes_before = vm->bytes_allocated;

#ifdef DEBUG_LOG_GC
	vm_print(vm, "--- gc begin ---\n");
	const size_t before = vm->bytes_allocated;
#endif

	mark_roots(vm);
	const uint64_t mark_roots_end_ns = gc_now_ns();
	trace_references(vm);
	const uint64_t trace_end_ns = gc_now_ns();
	table_remove_white(vm, &vm->strings); // Clean up string table
	const uint64_t remove_white_end_ns = gc_now_ns();
	vm->gc_last_objects_before_sweep = vm->object_count;
	sweep(vm);
	const uint64_t sweep_end_ns = gc_now_ns();
	vm->gc_last_objects_after_sweep = vm->object_count;
	vm->next_gc = compute_next_gc_threshold(vm);
	vm->gc_last_bytes_after = vm->bytes_allocated;
	vm->gc_last_bytes_freed = vm->gc_last_bytes_before - vm->gc_last_bytes_after;
	vm->gc_last_next_gc = vm->next_gc;
	vm->gc_last_objects_freed = vm->gc_last_objects_before_sweep - vm->gc_last_objects_after_sweep;
	vm->gc_last_live_objects = vm->object_count;
	vm->gc_last_pool_capacity = slab_pool_capacity(vm->slab_24) + slab_pool_capacity(vm->slab_32) +
								slab_pool_capacity(vm->slab_48) + slab_pool_capacity(vm->slab_64);
	if (vm->gc_last_pool_capacity < vm->gc_last_live_objects) {
		vm->gc_last_pool_capacity = vm->gc_last_live_objects;
	}
	vm->gc_last_strings_count = vm->strings.count;
	vm->gc_last_strings_capacity = vm->strings.capacity;
	vm->gc_last_strings_tombstones = table_tombstone_count(&vm->strings);
	vm->gc_last_mark_roots_ns = mark_roots_end_ns - phase_start_ns;
	vm->gc_last_trace_ns = trace_end_ns - mark_roots_end_ns;
	vm->gc_last_remove_white_ns = remove_white_end_ns - trace_end_ns;
	vm->gc_last_sweep_ns = sweep_end_ns - remove_white_end_ns;
	vm->gc_last_total_ns = sweep_end_ns - gc_start_ns;
	vm->gc_collections++;
	vm->gc_mark_roots_ns += vm->gc_last_mark_roots_ns;
	vm->gc_trace_ns += vm->gc_last_trace_ns;
	vm->gc_remove_white_ns += vm->gc_last_remove_white_ns;
	vm->gc_sweep_ns += vm->gc_last_sweep_ns;
	vm->gc_total_ns += vm->gc_last_total_ns;

#ifdef DEBUG_LOG_GC
	vm_print(vm, "--- gc end ---\n");
	vm_print(vm, "    collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm->bytes_allocated, before,
			 vm->bytes_allocated, vm->next_gc);
#endif
}
