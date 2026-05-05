#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "table.h"
#include "utf8.h"
#include "value.h"
#include "vm.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "garbage_collector.h"
#include "object.h"
#include "panic.h"

/**
 * @brief Allocates a new object of the specified type.
 *
 * @param vm The virtual machine.
 * @param size The size of the object to allocate in bytes.
 * @param type The type of the object being allocated (ObjectType enum).
 *
 * @return A pointer to the newly allocated and initialized Object.
 */

CruxObject *allocate_pooled_object(CruxVM *vm, const size_t size, const ObjectType type)
{
	CruxObject *object = allocate_object_with_gc(vm, size);
	object_init(object, vm->objects, type, false, false);
	vm->object_count++;
	vm->objects = object;

#ifdef DEBUG_LOG_GC
	vm_print(vm, "%p allocate %zu for %d\n", (void *)object, size, type);
#endif

	return object;
}

/**
 * @brief Calculates the next power of 2 capacity for a collection.
 *
 * @param n The desired minimum capacity.
 *
 * @return The next power of 2 capacity greater than or equal to `n`, or
 * `UINT16_MAX - 1` if `n` is close to the maximum.
 */
static uint32_t calculateCollectionCapacity(uint32_t n)
{
	if (n >= UINT16_MAX - 1) {
		return UINT16_MAX - 1;
	}

	if (n < 8)
		return 8;
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

/**
 * @brief Generates a hash code for a CruxValue.
 *
 * @param value The CruxValue to hash.
 *
 * @return A 32-bit hash code for the CruxValue.
 */
static uint32_t hashValue(const CruxValue value)
{
	if (IS_CRUX_STRING(value)) {
		return AS_CRUX_STRING(value)->hash;
	}
	if (IS_NUMERIC(value)) {
		double num = TO_DOUBLE(value);
		if (num == 0.0)
			return 0u;
		uint64_t bits;
		memcpy(&bits, &num, sizeof(bits));
		return (uint32_t)(bits ^ bits >> 32);
	}
	if (IS_BOOL(value)) {
		return AS_BOOL(value) ? 1u : 0u;
	}
	if (IS_NIL(value)) {
		return 4321u;
	}
	return 0u;
}

int sprint_type_to(char *buffer, size_t size, const CruxValue value)
{
	if (size == 0)
		return 0;
	int written = 0;
#define APPEND(...)                                                                                                    \
	do {                                                                                                               \
		if (size > (size_t)written) {                                                                                  \
			int _w = snprintf(buffer + written, size - written, __VA_ARGS__);                                          \
			if (_w > 0)                                                                                                \
				written += _w;                                                                                         \
		}                                                                                                              \
	} while (0)

	if (IS_INT(value)) {
		APPEND("Int");
		return written;
	}
	if (IS_FLOAT(value)) {
		APPEND("Float");
		return written;
	}
	if (IS_BOOL(value)) {
		APPEND("Bool");
		return written;
	}
	if (IS_NIL(value)) {
		APPEND("Nil");
		return written;
	}
	switch (OBJECT_TYPE(value)) {
	case OBJECT_STRING:
		APPEND("String");
		break;
	case OBJECT_FUNCTION:
	case OBJECT_NATIVE_CALLABLE:
	case OBJECT_CLOSURE:
		APPEND("Function");
		break;
	case OBJECT_UPVALUE: {
		const ObjectUpvalue *upvalue = AS_CRUX_UPVALUE(value);
		written += sprint_type_to(buffer + written, size - written, upvalue->closed);
		break;
	}
	case OBJECT_ARRAY: {
		const ObjectArray *array = AS_CRUX_ARRAY(value);
		if (array->size > 0) {
			APPEND("Array[");
			written += sprint_type_to(buffer + written, size - written, array->values[0]);
			APPEND("]");
		} else {
			APPEND("Array");
		}
		break;
	}
	case OBJECT_TABLE:
		APPEND("Table");
		break;
	case OBJECT_ERROR:
		APPEND("Error");
		break;
	case OBJECT_RESULT: {
		const ObjectResult *result = AS_CRUX_RESULT(value);
		if (result->is_ok) {
			APPEND("Result[");
			written += sprint_type_to(buffer + written, size - written, result->as.value);
			APPEND("]");
		} else {
			APPEND("Result[Error]");
		}
		break;
	}
	case OBJECT_RANDOM:
		APPEND("Random");
		break;
	case OBJECT_FILE:
		APPEND("File");
		break;
	case OBJECT_VECTOR: {
		const ObjectVector *vector = AS_CRUX_VECTOR(value);
		APPEND("Vector[%d]", vector->dimensions);
		break;
	}
	case OBJECT_MODULE_RECORD: {
		APPEND("Module");
		break;
	}
	case OBJECT_STRUCT_INSTANCE: {
		const ObjectStructInstance *instance = AS_CRUX_STRUCT_INSTANCE(value);
		APPEND("%s instance", instance->struct_type->name->chars);
		break;
	}
	case OBJECT_STRUCT: {
		const ObjectStruct *struct_ = AS_CRUX_STRUCT(value);
		APPEND("Struct %s", struct_->name->chars);
		break;
	}
	case OBJECT_COMPLEX: {
		APPEND("Complex");
		break;
	}
	case OBJECT_MATRIX: {
		const ObjectMatrix *matrix = AS_CRUX_MATRIX(value);
		APPEND("Matrix[%d, %d]", matrix->row_dim, matrix->col_dim);
		break;
	}
	case OBJECT_RANGE: {
		APPEND("Range");
		break;
	}
	case OBJECT_ITERATOR: {
		APPEND("Iterator");
		break;
	}
	case OBJECT_TUPLE: {
		APPEND("Tuple");
		break;
	}
	case OBJECT_BUFFER: {
		APPEND("Buffer");
		break;
	}

	case OBJECT_OPTION: {
		const ObjectOption *option = AS_CRUX_OPTION(value);
		if (option->is_some) {
			APPEND("Option[");
			written += sprint_type_to(buffer + written, size - written, option->value);
			APPEND("]");
		} else {
			APPEND("Option[None]");
		}
		break;
	}
	case OBJECT_ENUM: {
		APPEND("Enum");
		break;
	}
	case OBJECT_COROUTINE: {
		APPEND("Coroutine");
		break;
	}
	default:
		APPEND("Unknown");
	}
#undef APPEND
	return written;
}

void print_type_to(CruxVM *vm, const CruxValue value)
{
	char buffer[256];
	sprint_type_to(buffer, sizeof(buffer), value);
	vm_print(vm, "%s", buffer);
}

ObjectUpvalue *new_upvalue(CruxVM *vm, CruxValue *slot)
{
	ObjectUpvalue *upvalue = ALLOCATE_OBJECT(vm, ObjectUpvalue, OBJECT_UPVALUE);
	upvalue->location = slot;
	upvalue->next = NULL;
	upvalue->closed = NIL_VAL;
	return upvalue;
}

ObjectClosure *new_closure(CruxVM *vm, ObjectFunction *function)
{
	push(vm->current_module_record, OBJECT_VAL(function));
	ObjectUpvalue **upvalues = ALLOCATE(vm, ObjectUpvalue *, function->upvalue_count);
	push(vm->current_module_record, OBJECT_VAL(upvalues));
	for (int i = 0; i < function->upvalue_count; i++) {
		upvalues[i] = NULL;
	}

	ObjectClosure *closure = ALLOCATE_OBJECT(vm, ObjectClosure, OBJECT_CLOSURE);
	pop(vm->current_module_record);
	pop(vm->current_module_record);
	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalue_count = function->upvalue_count;
	return closure;
}

/**
 * @brief Allocates a new string object. calculates and stores the
 * string's hash value and interns the string in the CruxVM's string table.
 *
 * @param vm The virtual machine.
 * @param chars The character array for the string. This memory is assumed to be
 * managed externally and copied.
 * @param length The length of the string.
 * @param hash The pre-calculated hash value of the string.
 *
 * @return A pointer to the newly created and interned ObjectString.
 */
static ObjectString *allocate_string(CruxVM *vm, utf8_int8_t *chars, const uint32_t byte_length, const uint32_t hash)
{
	ObjectString *string = ALLOCATE_OBJECT(vm, ObjectString, OBJECT_STRING);
	string->byte_length = byte_length;
	string->code_point_length = utf8len(chars);
	string->chars = chars;
	string->hash = hash;
	// intern the string
	push(vm->current_module_record, OBJECT_VAL(string));
	table_set(vm, &vm->strings, string, NIL_VAL);
	pop(vm->current_module_record);
	return string;
}

/**
 * @brief Calculates the hash value of a C-style string.
 *
 * This function implements the FNV-1a hash algorithm to generate a hash
 * code for a given C-style string.
 *
 * @param key The null-terminated C-style string to hash.
 * @param length The length of the string (excluding null terminator).
 *
 * @return A 32-bit hash code for the string.
 */
uint32_t hash_string(const char *key, const size_t length)
{
	static const uint32_t FNV_OFFSET_BIAS = 2166136261u;
	static const uint32_t FNV_PRIME = 16777619u;

	uint32_t hash = FNV_OFFSET_BIAS;
	for (size_t i = 0; i < length; i++) {
		hash ^= (uint8_t)key[i];
		hash *= FNV_PRIME;
	}
	return hash;
}

ObjectString *copy_string(CruxVM *vm, const char *chars, const uint32_t length)
{
	const uint32_t hash = hash_string(chars, length);

	ObjectString *interned = table_find_string(&vm->strings, chars, length, hash);
	if (interned != NULL)
		return interned;

	char *heapChars = ALLOCATE(vm, char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0'; // terminating the string because it is not
							  // terminated in the source
	return allocate_string(vm, heapChars, length, hash);
}

void print_error_type_to(CruxVM *vm, const ErrorType type)
{
	switch (type) {
	case SYNTAX:
		vm_print(vm, "syntax");
		break;
	case MATH:
		vm_print(vm, "math");
		break;
	case BOUNDS:
		vm_print(vm, "bounds");
		break;
	case RUNTIME:
		vm_print(vm, "runtime");
		break;
	case TYPE:
		vm_print(vm, "type");
		break;
	case LOOP_EXTENT:
		vm_print(vm, "loop");
		break;
	case LIMIT:
		vm_print(vm, "limit");
		break;
	case BRANCH_EXTENT:
		vm_print(vm, "branch");
		break;
	case CLOSURE_EXTENT:
		vm_print(vm, "closure");
		break;
	case LOCAL_EXTENT:
		vm_print(vm, "local");
		break;
	case ARGUMENT_EXTENT:
		vm_print(vm, "argument");
		break;
	case NAME:
		vm_print(vm, "name");
		break;
	case COLLECTION_EXTENT:
		vm_print(vm, "collection");
		break;
	case VARIABLE_EXTENT:
		vm_print(vm, "variable");
		break;
	case RETURN_EXTENT:
		vm_print(vm, "return");
		break;
	case ARGUMENT_MISMATCH:
		vm_print(vm, "argument mismatch");
		break;
	case STACK_OVERFLOW:
		vm_print(vm, "stack overflow");
		break;
	case COLLECTION_GET:
		vm_print(vm, "collection get");
		break;
	case COLLECTION_SET:
		vm_print(vm, "collection set");
		break;
	case MEMORY:
		vm_print(vm, "memory");
		break;
	case VALUE:
		vm_print(vm, "value");
		break;
	case ASSERT:
		vm_print(vm, "assert");
		break;
	case IMPORT_EXTENT:
		vm_print(vm, "import");
		break;
	case IO:
		vm_print(vm, "io");
		break;
	case IMPORT:
		vm_print(vm, "import");
		break;
	}
}

/**
 * @brief Prints the name of a function object.
 *
 * This static helper function prints the name of a function object to the
 * console, used for debugging and representation. If the function is
 * anonymous (name is NULL), it prints "<script>".
 *
 * @param vm The CruxVM
 * @param function The ObjectFunction to print the name of.
 */
static void print_function_to(CruxVM *vm, const ObjectFunction *function)
{
	if (function->name == NULL) {
		vm_print(vm, "<script>");
		return;
	}
	vm_print(vm, "<fn %s>", function->name->chars);
}

static void print_array_to(CruxVM *vm, const CruxValue *values, const uint32_t size)
{
	vm_print(vm, "[");
	for (uint32_t i = 0; i < size; i++) {
		print_value_to(vm, values[i], true);
		if (i != size - 1) {
			vm_print(vm, ", ");
		}
	}
	vm_print(vm, "]");
}

static void print_table_to(CruxVM *vm, const ObjectTableEntry *entries, const uint32_t capacity, const uint32_t size,
						   bool is_set)
{
	uint32_t printed = 0;
	if (entries == NULL) {
		if (is_set) {
			vm_print(vm, "${}");
		} else {
			vm_print(vm, "{}");
		}
		return;
	}
	if (is_set) {
		vm_print(vm, "${");
	} else {
		vm_print(vm, "{");
	}
	for (uint32_t i = 0; i < capacity; i++) {
		if (entries[i].is_occupied) {
			print_value_to(vm, entries[i].key, true);
			vm_print(vm, ":");
			print_value_to(vm, entries[i].value, true);
			if (printed != size - 1) {
				vm_print(vm, ", ");
			}
			printed++;
		}
	}
	vm_print(vm, "}");
}

static void print_struct_instance_to(CruxVM *vm, const ObjectStructInstance *instance)
{
	vm_print(vm, "{");
	int printed = 0;
	const ObjectStruct *type = instance->struct_type;
	if (instance->fields == NULL) {
		vm_print(vm, "}");
		return;
	}
	for (int i = 0; i < type->fields.capacity; i++) {
		if (type->fields.entries[i].key != NULL) {
			const uint16_t index = (uint16_t)AS_INT(type->fields.entries[i].value);
			const ObjectString *fieldName = type->fields.entries[i].key;
			vm_print(vm, "%s: ", fieldName->chars);
			print_value_to(vm, instance->fields[index], true);
			if (printed != type->fields.count - 1) {
				vm_print(vm, ", ");
			}
			printed++;
		}
	}
	vm_print(vm, "}");
}

static void print_result_to(CruxVM *vm, const ObjectResult *result)
{
	if (result->is_ok) {
		vm_print(vm, "Ok<");
		print_type_to(vm, result->as.value);
		vm_print(vm, ">");
	} else {
		vm_print(vm, "Err<");
		print_error_type_to(vm, result->as.error->type);
		vm_print(vm, ">");
	}
}

void print_object_to(CruxVM *vm, const CruxValue value, const bool in_collection)
{
	switch (OBJECT_TYPE(value)) {
	case OBJECT_STRING: {
		if (in_collection) {
			vm_print(vm, "'%s'", AS_C_STRING(value));
			break;
		}
		vm_print(vm, "%s", AS_C_STRING(value));
		break;
	}
	case OBJECT_FUNCTION: {
		print_function_to(vm, AS_CRUX_FUNCTION(value));
		break;
	}
	case OBJECT_NATIVE_CALLABLE: {
		const ObjectNativeCallable *native = AS_CRUX_NATIVE_CALLABLE(value);
		if (native->name != NULL) {
			vm_print(vm, "<native callable %s>", native->name->chars);
		} else {
			vm_print(vm, "<native callable>");
		}
		break;
	}
	case OBJECT_CLOSURE: {
		print_function_to(vm, AS_CRUX_CLOSURE(value)->function);
		break;
	}
	case OBJECT_UPVALUE: {
		print_value_to(vm, value, false);
		break;
	}
	case OBJECT_ARRAY: {
		const ObjectArray *array = AS_CRUX_ARRAY(value);
		print_array_to(vm, array->values, array->size);
		break;
	}
	case OBJECT_TABLE: {
		const ObjectTable *table = AS_CRUX_TABLE(value);
		print_table_to(vm, table->entries, table->capacity, table->size, false);
		break;
	}
	case OBJECT_ERROR: {
		vm_print(vm, "<error ");
		print_error_type_to(vm, AS_CRUX_ERROR(value)->type);
		vm_print(vm, ">");
		break;
	}
	case OBJECT_RESULT: {
		print_result_to(vm, AS_CRUX_RESULT(value));
		break;
	}
	case OBJECT_RANDOM: {
		vm_print(vm, "<random>");
		break;
	}
	case OBJECT_FILE: {
		vm_print(vm, "<file>");
		break;
	}
	case OBJECT_MODULE_RECORD: {
		vm_print(vm, "<module record>");
		break;
	}
	case OBJECT_STRUCT: {
		vm_print(vm, "<struct type %s>", AS_CRUX_STRUCT(value)->name->chars);
		break;
	}
	case OBJECT_STRUCT_INSTANCE: {
		print_struct_instance_to(vm, AS_CRUX_STRUCT_INSTANCE(value));
		break;
	}
	case OBJECT_VECTOR: {
		const ObjectVector *vector = AS_CRUX_VECTOR(value);
		vm_print(vm, "Vector(%d)[", vector->dimensions);
		const double *comp = VECTOR_COMPONENTS(vector);
		for (uint32_t i = 0; i < vector->dimensions; i++) {
			vm_print(vm, "%.17g", comp[i]);
			if (i != vector->dimensions - 1) {
				vm_print(vm, ", ");
			}
		}
		vm_print(vm, "]");
		break;
	}
	case OBJECT_MATRIX: {
		const ObjectMatrix *mat = AS_CRUX_MATRIX(value);
		vm_print(vm, "Matrix(%ux%u)\n", mat->row_dim, mat->col_dim);
		for (uint16_t i = 0; i < mat->col_dim; i++) {
			for (uint16_t j = 0; j < mat->row_dim; j++) {
				if (j == 0) {
					vm_print(vm, "| ");
				}
				vm_print(vm, "%.17g", mat->data[i * mat->row_dim + j]);
				if (j != mat->row_dim - 1) {
					vm_print(vm, ", ");
				}
				if (j == mat->row_dim - 1) {
					vm_print(vm, " |");
				}
			}
			if (i != mat->col_dim - 1) {
				vm_print(vm, "\n");
			}
		}
		break;
	}
	case OBJECT_COMPLEX: {
		const ObjectComplex *c = AS_CRUX_COMPLEX(value);
		if (c->imag >= 0.0) {
			vm_print(vm, "%.17g+%.17gi", c->real, c->imag);
		} else {
			vm_print(vm, "%.17g%.17gi", c->real, c->imag);
		}
		break;
	}

	case OBJECT_BUFFER: {
		vm_print(vm, "<Buffer>");
		break;
	}
	case OBJECT_TUPLE: {
		const ObjectTuple *tuple = AS_CRUX_TUPLE(value);
		vm_print(vm, "$[");
		for (uint32_t i = 0; i < tuple->size; i++) {
			if (i != 0) {
				vm_print(vm, ", ");
			}
			print_value_to(vm, tuple->elements[i], true);
		}
		vm_print(vm, "]");
		break;
	}
	case OBJECT_RANGE: {
		const ObjectRange *range = AS_CRUX_RANGE(value);
		vm_print(vm, "<Range(%d..%d..%d)>", range->start, range->step, range->end);
		break;
	}
	case OBJECT_ITERATOR: {
		vm_print(vm, "<iterator>");
		break;
	}
	case OBJECT_TYPE_RECORD: {
		vm_print(vm, "<TypeRecord>");
		break;
	}
	case OBJECT_TYPE_TABLE: {
		vm_print(vm, "<TypeTable>");
		break;
	}
	case OBJECT_OPTION: {
		vm_print(vm, "<Option>");
		break;
	}
	case OBJECT_ENUM: {
		vm_print(vm, "<Enum>");
		break;
	}
	case OBJECT_COROUTINE: {
		vm_print(vm, "<Coroutine>");
		break;
	}
	case SENTINEL_OBJECT_COUNT:
		vm_print(vm, "<SENTINEL_OBJECT_COUNT>");
		break;
	}
}

void print_value_to(CruxVM *vm, const CruxValue value, const bool inCollection)
{
	if (IS_BOOL(value)) {
		vm_print(vm, AS_BOOL(value) ? "true" : "false");
	} else if (IS_NIL(value)) {
		vm_print(vm, "nil");
	} else if (IS_FLOAT(value)) {
		vm_print(vm, "%.17g", AS_FLOAT(value));
	} else if (IS_INT(value)) {
		vm_print(vm, "%d", AS_INT(value));
	} else if (IS_CRUX_OBJECT(value)) {
		print_object_to(vm, value, inCollection);
	}
}

void print_value(CruxVM *vm, const CruxValue value, const bool inCollection)
{
	print_value_to(vm, value, inCollection);
}

void print_object(CruxVM *vm, const CruxValue value, const bool in_collection)
{
	print_object_to(vm, value, in_collection);
}

ObjectString *take_string(CruxVM *vm, char *chars, const uint32_t length)
{
	const uint32_t hash = hash_string(chars, length);

	ObjectString *interned = table_find_string(&vm->strings, chars, length, hash);
	if (interned != NULL) {
		// free the string that was passed to us.
		FREE_ARRAY(vm, utf8_int8_t, chars, length + 1);
		return interned;
	}

	return allocate_string(vm, chars, length, hash);
}

ObjectString *to_string(CruxVM *vm, const CruxValue value)
{
	if (!IS_CRUX_OBJECT(value)) {
		char buffer[32];
		if (IS_FLOAT(value)) {
			const double num = AS_FLOAT(value);
			snprintf(buffer, sizeof(buffer), "%.17g", num);
		} else if (IS_INT(value)) {
			const int32_t num = AS_INT(value);
			snprintf(buffer, sizeof(buffer), "%d", num);
		} else if (IS_BOOL(value)) {
			strcpy(buffer, AS_BOOL(value) ? "true" : "false");
		} else if (IS_NIL(value)) {
			strcpy(buffer, "nil");
		}
		return copy_string(vm, buffer, (int)strlen(buffer));
	}

	switch (OBJECT_TYPE(value)) {
	case OBJECT_STRING:
		return AS_CRUX_STRING(value);

	case OBJECT_MODULE_RECORD: {
		return copy_string(vm, "<module>", 8);
	}

	case OBJECT_FUNCTION: {
		const ObjectFunction *function = AS_CRUX_FUNCTION(value);
		if (function->name == NULL) {
			return copy_string(vm, "<script>", 8);
		}
		char buffer[64];
		const int length = snprintf(buffer, sizeof(buffer), "<fn %s>", function->name->chars);
		return copy_string(vm, buffer, length);
	}

	case OBJECT_NATIVE_CALLABLE: {
		const ObjectNativeCallable *native = AS_CRUX_NATIVE_CALLABLE(value);
		if (native->name != NULL) {
			const char *start = "<native fn ";
			const char *end = ">";
			char *buffer = ALLOCATE(vm, char, strlen(start) + strlen(end) + native->name->byte_length + 1);
			strcpy(buffer, start);
			strcat(buffer, native->name->chars);
			strcat(buffer, end);
			ObjectString *result = take_string(vm, buffer, strlen(buffer));
			FREE_ARRAY(vm, char, buffer, strlen(buffer) + 1);
			return result;
		}
		return copy_string(vm, "<native fn>", 11);
	}

	case OBJECT_CLOSURE: {
		const ObjectFunction *function = AS_CRUX_CLOSURE(value)->function;
		if (function->name == NULL) {
			return copy_string(vm, "<script>", 8);
		}
		char buffer[256];
		const int length = snprintf(buffer, sizeof(buffer), "<fn %s>", function->name->chars);
		return copy_string(vm, buffer, length);
	}

	case OBJECT_UPVALUE: {
		return copy_string(vm, "<upvalue>", 9);
	}

	case OBJECT_ARRAY: {
		const ObjectArray *array = AS_CRUX_ARRAY(value);
		size_t bufSize = 2; // [] minimum
		for (uint32_t i = 0; i < array->size; i++) {
			const ObjectString *element = to_string(vm, array->values[i]);
			bufSize += element->byte_length + 2; // element + ", "
		}

		char *buffer = ALLOCATE(vm, char, bufSize);
		char *ptr = buffer;
		*ptr++ = '[';

		for (uint32_t i = 0; i < array->size; i++) {
			if (i > 0) {
				*ptr++ = ',';
				*ptr++ = ' ';
			}
			const ObjectString *element = to_string(vm, array->values[i]);
			memcpy(ptr, element->chars, element->byte_length);
			ptr += element->byte_length;
		}
		*ptr++ = ']';

		ObjectString *result = take_string(vm, buffer, ptr - buffer);
		return result;
	}

	case OBJECT_TABLE: {
		const ObjectTable *table = AS_CRUX_TABLE(value);
		size_t bufSize = 2; // {} minimum
		for (uint32_t i = 0; i < table->capacity; i++) {
			if (table->entries[i].is_occupied) {
				const ObjectString *k = to_string(vm, table->entries[i].key);
				const ObjectString *v = to_string(vm, table->entries[i].value);
				bufSize += k->byte_length + v->byte_length + 4; // key:value
			}
		}

		char *buffer = ALLOCATE(vm, char, bufSize);
		char *ptr = buffer;
		*ptr++ = '{';

		bool first = true;
		for (uint32_t i = 0; i < table->capacity; i++) {
			if (table->entries[i].is_occupied) {
				if (!first) {
					*ptr++ = ',';
					*ptr++ = ' ';
				}
				first = false;

				const ObjectString *key = to_string(vm, table->entries[i].key);
				const ObjectString *val = to_string(vm, table->entries[i].value);

				memcpy(ptr, key->chars, key->byte_length);
				ptr += key->byte_length;
				*ptr++ = ':';
				memcpy(ptr, val->chars, val->byte_length);
				ptr += val->byte_length;
			}
		}
		*ptr++ = '}';

		ObjectString *result = take_string(vm, buffer, ptr - buffer);
		return result;
	}

	case OBJECT_ERROR: {
		const ObjectError *error = AS_CRUX_ERROR(value);
		char buffer[1024];
		const int length = snprintf(buffer, sizeof(buffer), "<Error: %s>", error->message->chars);
		return copy_string(vm, buffer, length);
	}

	case OBJECT_RESULT: {
		const ObjectResult *result = AS_CRUX_RESULT(value);
		if (result->is_ok) {
			return copy_string(vm, "<Ok>", 4);
		}
		return copy_string(vm, "<Err>", 5);
	}

	case OBJECT_RANDOM: {
		return copy_string(vm, "<Random>", 8);
	}

	case OBJECT_FILE: {
		return copy_string(vm, "<File>", 6);
	}

	case OBJECT_STRUCT: {
		return copy_string(vm, "<Struct>", 8);
	}

	case OBJECT_STRUCT_INSTANCE: {
		return copy_string(vm, "<Struct Instance>", 17);
	}

	case OBJECT_VECTOR: {
		return copy_string(vm, "<Vector[]>", 10);
	}

	case OBJECT_COMPLEX: {
		return copy_string(vm, "<Complex>", 9);
	}

	case OBJECT_MATRIX: {
		return copy_string(vm, "<Matrix>", 8);
	}

	case OBJECT_BUFFER: {
		return copy_string(vm, "<Buffer>", 8);
	}
	case OBJECT_TUPLE: {
		return copy_string(vm, "<Tuple>", 7);
	}
	case OBJECT_RANGE: {
		return copy_string(vm, "<Range>", 7);
	}
	case OBJECT_ITERATOR: {
		return copy_string(vm, "<Iterator>", 10);
	}
	case OBJECT_OPTION: {
		return copy_string(vm, "<Option>", 8);
	}
	case OBJECT_ENUM: {
		return copy_string(vm, "<Enum>", 6);
	}
	case OBJECT_COROUTINE: {
		return copy_string(vm, "<Coroutine>", 11);
	}

	default:
		return copy_string(vm, "<Crux Object>", 13);
	}
}

ObjectFunction *new_function(CruxVM *vm)
{
	ObjectFunction *function = ALLOCATE_OBJECT(vm, ObjectFunction, OBJECT_FUNCTION);
	function->arity = 0;
	function->name = NULL;
	function->upvalue_count = 0;
	init_chunk(&function->chunk);
	function->module_record = vm->current_module_record;
	return function;
}

/**
 *
 * @param vm The Virtual Machine
 * @param function The executable function
 * @param arity The number of arguments
 * @param name The name of the function
 * @param arg_types The types of the function arguments. This should be an owned
 * array
 * @param return_type The type of the function's returned value
 * @return The GC owned object
 */
ObjectNativeCallable *new_native_callable(CruxVM *vm, const CruxCallable function, const int arity, ObjectString *name,
										  ObjectTypeRecord **arg_types, ObjectTypeRecord *return_type)
{
	push(vm->current_module_record, OBJECT_VAL(name));
	ObjectNativeCallable *native = ALLOCATE_OBJECT(vm, ObjectNativeCallable, OBJECT_NATIVE_CALLABLE);
	pop(vm->current_module_record);
	native->function = function;
    native->foreign_fn = NULL;
	native->arity = arity;
	native->name = name;
	if (arg_types != NULL && arity > 0) {
		native->arg_types = arg_types;
	} else {
		native->arg_types = NULL;
	}
	native->return_type = return_type;
	return native;
}

ObjectTable *new_object_table(CruxVM *vm, const int element_count)
{
	ObjectTable *table = ALLOCATE_OBJECT(vm, ObjectTable, OBJECT_TABLE);
	push(vm->current_module_record, OBJECT_VAL(table));
	table->size = 0;
	table->entries = NULL;
	const uint32_t newCapacity = element_count < 16 ? 16 : calculateCollectionCapacity(element_count);
	table->capacity = newCapacity;
	table->entries = ALLOCATE(vm, ObjectTableEntry, table->capacity);
	for (uint32_t i = 0; i < table->capacity; i++) {
		table->entries[i].value = NIL_VAL;
		table->entries[i].key = NIL_VAL;
		table->entries[i].is_occupied = false;
	}
	pop(vm->current_module_record);
	return table;
}

void free_object_table(CruxVM *vm, ObjectTable *table)
{
	FREE_ARRAY(vm, ObjectTableEntry, table->entries, table->capacity);
	table->entries = NULL;
	table->capacity = 0;
	table->size = 0;
}

ObjectFile *new_object_file(CruxVM *vm, ObjectString *path, ObjectString *mode)
{
	// TODO: Make this open files in non existent directories
	push(vm->current_module_record, OBJECT_VAL(path));
	push(vm->current_module_record, OBJECT_VAL(mode));
	ObjectFile *file = ALLOCATE_OBJECT(vm, ObjectFile, OBJECT_FILE);
	pop(vm->current_module_record);
	pop(vm->current_module_record);
	file->path = path;
	file->mode = mode;

	/* On Windows, always open in binary mode to avoid CRLF translation
	   issues with ftell/fseek when determining file size. */
#ifdef _WIN32
	char bin_mode[8];
	snprintf(bin_mode, sizeof(bin_mode), "%sb", mode->chars);
	file->file = fopen(path->chars, bin_mode);
#else
	file->file = fopen(path->chars, mode->chars);
#endif

	file->is_open = file->file != NULL;
	file->position = 0;
	return file;
}

/**
 * @brief Finds an entry in an object table.
 *
 * @param entries The array of ObjectTableEntry.
 * @param capacity The capacity of the table's entry array.
 * @param key The key CruxValue to search for.
 *
 * @return A pointer to the ObjectTableEntry for the key, or a pointer to an
 * empty entry (possibly a tombstone) if the key is not found.
 */
static ObjectTableEntry *find_entry(ObjectTableEntry *entries, const uint16_t capacity, const CruxValue key)
{
	const uint32_t hash = hashValue(key);
	uint32_t index = hash & (capacity - 1);
	ObjectTableEntry *tombstone = NULL;

	while (1) {
		ObjectTableEntry *entry = &entries[index];
		if (!entry->is_occupied) {
			if (IS_NIL(entry->value)) {
				return tombstone != NULL ? tombstone : entry;
			}
			if (tombstone == NULL) {
				tombstone = entry;
			}
		} else if (values_equal(entry->key, key)) {
			return entry;
		}
		index = (index * 5 + 1) & (capacity - 1);
	}
}

/**
 * @brief Adjusts the capacity of an object table.
 *
 * @param vm The virtual machine.
 * @param table The ObjectTable to resize.
 * @param capacity The new capacity for the table.
 *
 * @return true if the capacity adjustment was successful, false otherwise
 * (e.g., memory allocation failure).
 */
static bool adjust_capacity(CruxVM *vm, ObjectTable *table, const int capacity)
{
	push(vm->current_module_record, OBJECT_VAL(table));
	ObjectTableEntry *entries = ALLOCATE(vm, ObjectTableEntry, capacity);
	pop(vm->current_module_record);
	if (entries == NULL) {
		return false;
	}

	for (int i = 0; i < capacity; i++) {
		entries[i].key = NIL_VAL;
		entries[i].value = NIL_VAL;
		entries[i].is_occupied = false;
	}

	table->size = 0;

	for (uint32_t i = 0; i < table->capacity; i++) {
		const ObjectTableEntry *entry = &table->entries[i];
		if (!entry->is_occupied) {
			continue;
		}

		ObjectTableEntry *destination = find_entry(entries, capacity, entry->key);

		destination->key = entry->key;
		destination->value = entry->value;
		destination->is_occupied = true;
		table->size++;
	}

	FREE_ARRAY(vm, ObjectTableEntry, table->entries, table->capacity);
	table->entries = entries;
	table->capacity = capacity;
	return true;
}

bool object_table_set(CruxVM *vm, ObjectTable *table, const CruxValue key, const CruxValue value)
{
	if (table->size + 1 > table->capacity * TABLE_MAX_LOAD) {
		const int capacity = GROW_CAPACITY(table->capacity);
		if (!adjust_capacity(vm, table, capacity)) {
			return false;
		}
	}

	ObjectTableEntry *entry = find_entry(table->entries, table->capacity, key);
	const bool isNewKey = !entry->is_occupied;

	if (isNewKey) {
		table->size++;
	}

	if (IS_CRUX_OBJECT(key))
		mark_value(vm, key);
	if (IS_CRUX_OBJECT(value))
		mark_value(vm, value);

	entry->key = key;
	entry->value = value;
	entry->is_occupied = true;

	return true;
}

bool object_table_remove(ObjectTable *table, const CruxValue key)
{
	if (!table) {
		return false;
	}
	ObjectTableEntry *entry = find_entry(table->entries, table->capacity, key);
	if (!entry->is_occupied) {
		return false;
	}
	entry->is_occupied = false;
	entry->key = NIL_VAL;
	entry->value = BOOL_VAL(false);
	table->size--;
	return true;
}

bool object_table_contains_key(ObjectTable *table, const CruxValue key)
{
	if (!table)
		return false;
	if (table->size == 0)
		return false;

	const ObjectTableEntry *entry = find_entry(table->entries, table->capacity, key);
	return entry->is_occupied;
}

bool entriesContainsKey(ObjectTableEntry *entries, const CruxValue key, const uint32_t capacity)
{
	if (!entries)
		return false;
	const ObjectTableEntry *entry = find_entry(entries, capacity, key);
	return entry->is_occupied;
}

bool object_table_get(ObjectTableEntry *entries, const uint32_t size, const uint32_t capacity, const CruxValue key,
					  CruxValue *value)
{
	if (size == 0) {
		return false;
	}

	const ObjectTableEntry *entry = find_entry(entries, capacity, key);
	if (!entry->is_occupied) {
		return false;
	}
	*value = entry->value;
	return true;
}

ObjectArray *new_array(CruxVM *vm, const uint32_t element_count)
{
	ObjectArray *array = ALLOCATE_OBJECT(vm, ObjectArray, OBJECT_ARRAY);
	push(vm->current_module_record, OBJECT_VAL(array));
	array->capacity = calculateCollectionCapacity(element_count);
	array->size = 0;
	array->values = ALLOCATE(vm, CruxValue, array->capacity);
	for (uint32_t i = 0; i < array->capacity; i++) {
		array->values[i] = NIL_VAL;
	}
	pop(vm->current_module_record);
	return array;
}

bool ensure_capacity(CruxVM *vm, ObjectArray *array, const uint32_t capacity_needed)
{
	if (capacity_needed <= array->capacity) {
		return true;
	}
	uint32_t newCapacity = GROW_CAPACITY(array->capacity);
	if (newCapacity < capacity_needed) {
		return false;
	}
	push(vm->current_module_record, OBJECT_VAL(array));
	CruxValue *newArray = GROW_ARRAY(vm, CruxValue, array->values, array->capacity, newCapacity);
	pop(vm->current_module_record);
	if (newArray == NULL) {
		return false;
	}
	for (uint32_t i = array->capacity; i < newCapacity; i++) {
		newArray[i] = NIL_VAL;
	}
	array->values = newArray;
	array->capacity = newCapacity;
	return true;
}

bool array_set(CruxVM *vm, const ObjectArray *array, const uint32_t index, const CruxValue value)
{
	if (index >= array->size) {
		return false;
	}
	if (IS_CRUX_OBJECT(value)) {
		mark_value(vm, value);
	}
	array->values[index] = value;
	return true;
}

bool array_add(CruxVM *vm, ObjectArray *array, const CruxValue value, const uint32_t index)
{
	if (!ensure_capacity(vm, array, array->size + 1)) {
		return false;
	}
	if (IS_CRUX_OBJECT(value)) {
		mark_value(vm, value);
	}
	array->values[index] = value;
	array->size++;
	return true;
}

bool array_add_back(CruxVM *vm, ObjectArray *array, const CruxValue value)
{
	if (!ensure_capacity(vm, array, array->size + 1)) {
		return false;
	}
	array->values[array->size] = value;
	array->size++;
	return true;
}

ObjectError *new_error(CruxVM *vm, ObjectString *message, const ErrorType type, const bool is_panic)
{
	push(vm->current_module_record, OBJECT_VAL(message));
	ObjectError *error = ALLOCATE_OBJECT(vm, ObjectError, OBJECT_ERROR);
	pop(vm->current_module_record);
	error->message = message;
	error->type = type;
	error->is_panic = is_panic;
	return error;
}

ObjectResult *new_ok_result(CruxVM *vm, const CruxValue value)
{
	push(vm->current_module_record, value);
	ObjectResult *result = ALLOCATE_OBJECT(vm, ObjectResult, OBJECT_RESULT);
	pop(vm->current_module_record);
	result->is_ok = true;
	result->as.value = value;
	return result;
}

ObjectResult *new_error_result(CruxVM *vm, ObjectError *error)
{
	push(vm->current_module_record, OBJECT_VAL(error));
	ObjectResult *result = ALLOCATE_OBJECT(vm, ObjectResult, OBJECT_RESULT);
	pop(vm->current_module_record);
	result->is_ok = false;
	result->as.error = error;
	return result;
}

ObjectRandom *new_random(CruxVM *vm)
{
	ObjectRandom *random = ALLOCATE_OBJECT(vm, ObjectRandom, OBJECT_RANDOM);
#ifdef _WIN32
	random->seed = (uint64_t)time(NULL) ^ GetTickCount() ^ (uint64_t)GetCurrentProcessId();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	random->seed = tv.tv_sec ^ tv.tv_usec ^ (uint64_t)getpid();
#endif
	return random;
}

void free_module_record(CruxVM *vm, ObjectModuleRecord *module_record)
{
	if (module_record == NULL)
		return;

	free_object_module_record(vm, module_record);
}

ObjectModuleRecord *new_object_module_record(CruxVM *vm, ObjectString *path, const bool is_repl, const bool is_main)
{
	GC_STATUS previous = vm->gc_status;
	vm->gc_status = PAUSED;

	ObjectModuleRecord *module_record = ALLOCATE_OBJECT(vm, ObjectModuleRecord, OBJECT_MODULE_RECORD);
	module_record->path = path;
	init_table(&module_record->global_names);
	init_table(&module_record->publics);

	module_record->types = new_type_table(vm, INITIAL_TYPE_TABLE_SIZE);
	module_record->state = STATE_LOADING;
	module_record->module_closure = NULL;
	module_record->enclosing_module = NULL;

	module_record->stack = (CruxValue *)malloc(STACK_MAX * sizeof(CruxValue));
	module_record->stack_top = module_record->stack;
	module_record->stack_limit = module_record->stack + STACK_MAX;
	module_record->open_upvalues = NULL;

	module_record->globals = NULL;
	module_record->global_count = 0;

	module_record->frames = (CallFrame *)malloc(FRAMES_MAX * sizeof(CallFrame));
	module_record->frame_count = 0;
	module_record->frame_capacity = FRAMES_MAX;

	module_record->is_main = is_main;
	module_record->is_repl = is_repl;

	module_record->owner = vm;

	vm->gc_status = previous;

	return module_record;
}

/**
 * Frees ObjectModuleRecord internals
 * @param vm The CruxVM
 * @param record The ObjectModuleRecord to free
 */
void free_object_module_record(CruxVM *vm, ObjectModuleRecord *record)
{
	free(record->frames);
	record->frames = NULL;
	free(record->stack);
	record->stack = NULL;
	free(record->globals);
	record->globals = NULL;
	record->global_count = 0;

	free_table(vm, &record->global_names);
	free_table(vm, &record->publics);
}

ObjectStruct *new_struct_type(CruxVM *vm, ObjectString *name)
{
	push(vm->current_module_record, OBJECT_VAL(name));
	ObjectStruct *structObject = ALLOCATE_OBJECT(vm, ObjectStruct, OBJECT_STRUCT);
	pop(vm->current_module_record);
	structObject->name = name;
	init_table(&structObject->fields);
	init_table(&structObject->methods);
	return structObject;
}

ObjectStructInstance *new_struct_instance(CruxVM *vm, ObjectStruct *struct_type, const uint16_t field_count)
{
	push(vm->current_module_record, OBJECT_VAL(struct_type));
	ObjectStructInstance *struct_instance = ALLOCATE_OBJECT(vm, ObjectStructInstance, OBJECT_STRUCT_INSTANCE);
	push(vm->current_module_record, OBJECT_VAL(struct_instance));
	struct_instance->struct_type = struct_type;
	struct_instance->fields = NULL;
	struct_instance->field_count = 0;
	struct_instance->fields = ALLOCATE(vm, CruxValue, field_count);
	for (int i = 0; i < field_count; i++) {
		struct_instance->fields[i] = NIL_VAL;
	}
	struct_instance->field_count = field_count;
	pop(vm->current_module_record);
	pop(vm->current_module_record);
	return struct_instance;
}

ObjectVector *new_vector(CruxVM *vm, const uint32_t dimensions)
{
	ObjectVector *vector = ALLOCATE_OBJECT(vm, ObjectVector, OBJECT_VECTOR);
	push(vm->current_module_record, OBJECT_VAL(vector));
	vector->dimensions = dimensions;
	if (vector->dimensions > 4) {
		vector->as.h_components = ALLOCATE(vm, double, dimensions);
	}
	pop(vm->current_module_record);
	return vector;
}

ObjectComplex *new_complex_number(CruxVM *vm, const double real, const double imaginary)
{
	ObjectComplex *complex_number = ALLOCATE_OBJECT(vm, ObjectComplex, OBJECT_COMPLEX);
	complex_number->real = real;
	complex_number->imag = imaginary;
	return complex_number;
}

ObjectMatrix *new_matrix(CruxVM *vm, const uint16_t row_dim, const uint16_t col_dim)
{
	ObjectMatrix *matrix = ALLOCATE_OBJECT(vm, ObjectMatrix, OBJECT_MATRIX);
	matrix->row_dim = row_dim;
	matrix->col_dim = col_dim;
	push(vm->current_module_record, OBJECT_VAL(matrix));
	matrix->data = ALLOCATE(vm, double, row_dim *col_dim);
	pop(vm->current_module_record);
	return matrix;
}

ObjectRange *new_range(CruxVM *vm, uint64_t start, uint64_t end, uint64_t step)
{
	ObjectRange *range = ALLOCATE_OBJECT(vm, ObjectRange, OBJECT_RANGE);
	range->start = start;
	range->end = end;
	range->step = step;
	return range;
}

ObjectIterator *new_iterator(CruxVM *vm, CruxValue iterable)
{
	ObjectIterator *iterator = ALLOCATE_OBJECT(vm, ObjectIterator, OBJECT_ITERATOR);
	iterator->iterable = iterable;
	iterator->index = 0;
	return iterator;
}

ObjectBuffer *new_buffer(CruxVM *vm, uint32_t buffer_size)
{
	ObjectBuffer *buffer = ALLOCATE_OBJECT(vm, ObjectBuffer, OBJECT_BUFFER);
	buffer->capacity = buffer_size;
	buffer->read_pos = 0;
	buffer->write_pos = 0;
	buffer->data = NULL;
	push(vm->current_module_record, OBJECT_VAL(buffer));
	buffer->data = ALLOCATE(vm, uint8_t, buffer->capacity);
	pop(vm->current_module_record);
	return buffer;
}

ObjectTuple *new_tuple(CruxVM *vm, uint32_t size)
{
	ObjectTuple *tuple = ALLOCATE_OBJECT(vm, ObjectTuple, OBJECT_TUPLE);
	tuple->elements = NULL;
	tuple->size = size;
	push(vm->current_module_record, OBJECT_VAL(tuple));
	tuple->elements = ALLOCATE(vm, CruxValue, size);
	pop(vm->current_module_record);
	return tuple;
}

ObjectTypeRecord *new_type_rec(CruxVM *vm, TypeMask base_type)
{
	ObjectTypeRecord *rec = ALLOCATE_OBJECT(vm, ObjectTypeRecord, OBJECT_TYPE_RECORD);
	rec->base_type = base_type;
	memset(&rec->as, 0, sizeof(rec->as));
	return rec;
}

ObjectTypeTable *new_type_table(CruxVM *vm, const int capacity)
{
	TypeEntry *entries = ALLOCATE(vm, TypeEntry, capacity);
	for (int i = 0; i < capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = NULL;
	}

	ObjectTypeTable *table = ALLOCATE_OBJECT(vm, ObjectTypeTable, OBJECT_TYPE_TABLE);
	table->capacity = capacity;
	table->count = 0;
	table->entries = entries;
	return table;
}

void mark_object_type_table(CruxVM *vm, ObjectTypeTable *table)
{
	for (int i = 0; i < table->capacity; i++) {
		mark_object(vm, (CruxObject *)table->entries[i].key);
		mark_object(vm, (CruxObject *)table->entries[i].value);
	}
}

/**
 * Adds a value to a set, validating hashability and deduplicating by key.
 */

bool validate_range_values(int32_t start, int32_t step, int32_t end, const char **error_message)
{
	if (step == 0) {
		if (error_message)
			*error_message = "<step> cannot be zero.";
		return false;
	}
	if (step > 0 && start > end) {
		if (error_message)
			*error_message = "<start> cannot be greater than <end> when <step> is positive.";
		return false;
	}
	if (step < 0 && start < end) {
		if (error_message)
			*error_message = "<start> cannot be less than <end> when <step> is negative.";
		return false;
	}
	return true;
}

uint32_t range_len(const ObjectRange *range)
{
	if (range->step > 0)
		return (range->end - range->start + range->step - 1) / range->step;
	else
		return (range->start - range->end - range->step - 1) / (-range->step);
}

bool range_contains(const ObjectRange *range, int32_t value)
{
	if (range->step > 0)
		return value >= range->start && value < range->end && (value - range->start) % range->step == 0;
	else
		return value <= range->start && value > range->end && (range->start - value) % (-range->step) == 0;
}

/**
 * Check if there is a next value in the current iterator
 * Returns true if there is a next value, false otherwise.
 * result is set to the next value if there is one.
 */
bool iterate_next(ObjectModuleRecord *module_record, ObjectIterator *iterator, CruxValue *result)
{
	const CruxValue iterable = iterator->iterable;

	if (!IS_CRUX_OBJECT(iterable)) {
		runtime_panic(module_record, TYPE, "Cannot iterate over a non-iterable value");
		return false;
	}

	switch (OBJECT_TYPE(iterable)) {
	case OBJECT_ARRAY: {
		const ObjectArray *array = AS_CRUX_ARRAY(iterable);
		if (iterator->index >= array->size) {
			return false;
		}
		*result = array->values[iterator->index++];
		return true;
	}
	case OBJECT_TUPLE: {
		const ObjectTuple *tuple = AS_CRUX_TUPLE(iterable);
		if (iterator->index >= tuple->size) {
			return false;
		}
		*result = tuple->elements[iterator->index++];
		return true;
	}
	case OBJECT_RANGE: {
		const ObjectRange *range = AS_CRUX_RANGE(iterable);
		const uint32_t len = range_len(range);
		if (iterator->index >= len) {
			return false;
		}
		*result = INT_VAL(range->start + (int32_t)iterator->index * range->step);
		iterator->index++;
		return true;
	}
	case OBJECT_STRING: {
		const ObjectString *string = AS_CRUX_STRING(iterable);
		if (iterator->index >= string->code_point_length) {
			return false;
		}
		const utf8_int8_t **starts = NULL;
		if (!collect_string_codepoint_starts(module_record->owner, string, &starts)) {
			runtime_panic(module_record, MEMORY, "Failed to iterate string.");
			return false;
		}

		const utf8_int8_t *start = starts[iterator->index];
		const utf8_int8_t *end = starts[iterator->index + 1];
		const int length = (int)(end - start);
		ObjectString *element = copy_string(module_record->owner, (const char *)start, length);
		FREE(module_record->owner, const utf8_int8_t *, starts);
		*result = OBJECT_VAL(element);
		iterator->index++;
		return true;
	}
	case OBJECT_BUFFER: {
		const ObjectBuffer *buffer = AS_CRUX_BUFFER(iterable);
		if (iterator->index >= buffer->write_pos) {
			return false;
		}
		*result = INT_VAL(buffer->data[iterator->index++]);
		return true;
	}
	case OBJECT_VECTOR: {
		const ObjectVector *vector = AS_CRUX_VECTOR(iterable);
		if (iterator->index >= vector->dimensions) {
			return false;
		}
		*result = FLOAT_VAL(VECTOR_COMPONENTS(vector)[iterator->index++]);
		return true;
	}
	case OBJECT_MATRIX: {
		const ObjectMatrix *matrix = AS_CRUX_MATRIX(iterable);
		if (iterator->index >= matrix->row_dim * matrix->col_dim) {
			return false;
		}
		*result = FLOAT_VAL(matrix->data[iterator->index++]);
		return true;
	}

	default:
		runtime_panic(module_record, TYPE,
					  "Cannot iterate over this value. Supported iterables are Array | Set | Tuple | String | Buffer | "
					  "Range | Vector | Matrix | Iterator.");
		return false;
	}
}

ObjectOption *new_option(CruxVM *vm, CruxValue value, bool is_some)
{
	ObjectOption *option = ALLOCATE_OBJECT(vm, ObjectOption, OBJECT_OPTION);
	option->value = value;
	option->is_some = is_some;
	return option;
}
