#include "native/tables.h"
#include "object.h"
#include "panic.h"

/**
 * Returns all values in a table as an array
 * arg0 -> table: Table
 * Returns Array[Any]
 */
CruxValue table_values_method(CruxVM *vm, const CruxValue *args)
{
	const ObjectTable *table = AS_CRUX_TABLE(args[0]);
	ObjectArray *values = new_array(vm, table->size);

	uint32_t last_insert = 0;

	for (uint32_t i = 0; i < table->capacity; i++) {
		const ObjectTableEntry entry = table->entries[i];
		if (entry.is_occupied) {
			values->values[last_insert] = entry.value;
			last_insert++;
		}
	}

	values->size = last_insert;

	return OBJECT_VAL(values);
}

/**
 * Returns all keys in a table as an array
 * arg0 -> table: Table
 * Returns Result<Array>
 */
CruxValue table_keys_method(CruxVM *vm, const CruxValue *args)
{
	const ObjectTable *table = AS_CRUX_TABLE(args[0]);

	ObjectArray *keys = new_array(vm, table->size);

	uint32_t last_insert = 0;

	for (uint32_t i = 0; i < table->capacity; i++) {
		const ObjectTableEntry entry = table->entries[i];
		if (entry.is_occupied) {
			keys->values[last_insert] = entry.key;
			last_insert++;
		}
	}

	keys->size = last_insert;

	return OBJECT_VAL(keys);
}

/**
 * Returns all key-value pairs in a table as an array of [key, value] arrays
 * arg0 -> table: Table
 * Returns Result<Array>
 */
CruxValue table_pairs_method(CruxVM *vm, const CruxValue *args)
{
	ObjectModuleRecord *module_record = vm->current_module_record;
	const ObjectTable *table = AS_CRUX_TABLE(args[0]);

	ObjectArray *pairs = new_array(vm, table->size);
	push(module_record, OBJECT_VAL(pairs));

	if (pairs == NULL) {
		const CruxValue res = MAKE_GC_SAFE_ERROR(vm, "Failed to allocate enough memory for <pairs> array.", MEMORY);
		pop(vm->current_module_record); // pop the array
		return res;
	}

	uint16_t lastInsert = 0;

	for (uint32_t i = 0; i < table->capacity; i++) {
		const ObjectTableEntry entry = table->entries[i];
		if (entry.is_occupied) {
			ObjectArray *pair = new_array(vm, 2);
			push(module_record, OBJECT_VAL(pair));
			if (pair == NULL) {
				CruxValue res = MAKE_GC_SAFE_ERROR(vm,
												   "Failed to allocate enough memory for "
												   "pair array",
												   MEMORY);
				pop(module_record);
				return res;
			}

			pair->values[0] = entry.key;
			pair->values[1] = entry.value;
			pair->size = 2;

			pairs->values[lastInsert] = OBJECT_VAL(pair);
			lastInsert++;
			pop(module_record);
		}
	}

	pairs->size = lastInsert;

	ObjectResult *res = new_ok_result(vm, OBJECT_VAL(pairs));
	pop(vm->current_module_record);
	return OBJECT_VAL(res);
}

/**
 * Removes a key-value pair from a table
 * arg0 -> table: Table
 * arg1 -> key: Hashable
 * Returns Result<Nil>
 */
CruxValue table_remove_method(CruxVM *vm, const CruxValue *args)
{
	ObjectTable *table = AS_CRUX_TABLE(args[0]);
	const CruxValue key = args[1];
	if (IS_CRUX_HASHABLE(key)) {
		const bool result = object_table_remove(table, key);
		if (!result) {
			return MAKE_GC_SAFE_ERROR(vm, "Failed to remove key: value pair from table.", VALUE);
		}
		return OBJECT_VAL(new_ok_result(vm, NIL_VAL));
	}
	return MAKE_GC_SAFE_ERROR(vm, "Unhashable type given as table key.", TYPE);
}

/**
 * Gets the value associated with a key from a table
 * arg0 -> table: Table
 * arg1 -> key: Hashable
 * Returns Result<Any>
 */
CruxValue table_get_method(CruxVM *vm, const CruxValue *args)
{
	const ObjectTable *table = AS_CRUX_TABLE(args[0]);
	const CruxValue key = args[1];
	if (IS_CRUX_HASHABLE(key)) {
		CruxValue value;
		const bool result = object_table_get(table->entries, table->size, table->capacity, key, &value);
		if (!result) {
			return MAKE_GC_SAFE_ERROR(vm, "Failed to get value from table.", VALUE);
		}
		return OBJECT_VAL(new_ok_result(vm, value));
	}
	return MAKE_GC_SAFE_ERROR(vm, "Unhashable type given as table key.", TYPE);
}

/**
 * Checks if a table contains a specific key
 * arg0 -> table: Table
 * arg1 -> key: Hashable
 * Returns Bool
 */
CruxValue table_has_key_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	ObjectTable *table = AS_CRUX_TABLE(args[0]);
	const CruxValue key = args[1];
	if (IS_CRUX_HASHABLE(key)) {
		const bool result = object_table_contains_key(table, key);
		return BOOL_VAL(result);
	}
	return BOOL_VAL(false);
}

/**
 * Gets a value from a table or returns a default if the key doesn't exist
 * arg0 -> table: Table
 * arg1 -> key: Any Hashable
 * arg2 -> default: Any
 * Returns Any
 */
CruxValue table_get_or_else_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectTable *table = AS_CRUX_TABLE(args[0]);
	const CruxValue key = args[1];
	const CruxValue defaultValue = args[2];
	if (IS_CRUX_HASHABLE(key)) {
		CruxValue value;
		const bool result = object_table_get(table->entries, table->size, table->capacity, key, &value);
		if (!result) {
			return defaultValue;
		}
		return value;
	}
	return defaultValue;
}
