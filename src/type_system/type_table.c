#include "common.h"
#include "type_system.h"

#define TYPE_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

static TypeEntry *type_find_entry(TypeEntry *entries, const int capacity, const ObjectString *key)
{
	uint32_t index = key->hash & (capacity - 1);
	TypeEntry *tombstone = NULL;
	for (;;) {
		TypeEntry *entry = &entries[index];

		if (entry->key == NULL) {
			if (entry->value == NULL) {
				return tombstone != NULL ? tombstone : entry;
			}
			if (tombstone == NULL)
				tombstone = entry;
		} else if (entry->key == key || compare_strings(entry->key, key)) {
			return entry;
		}

		index = (index + 1) & (capacity - 1);
	}
}

static void type_table_adjust_capacity(ObjectTypeTable *table, const int capacity)
{
	TypeEntry *entries = calloc(capacity, sizeof(TypeEntry));
	table->count = 0;
	for (int i = 0; i < table->capacity; i++) {
		const TypeEntry *entry = &table->entries[i];
		if (entry->key == NULL)
			continue;

		TypeEntry *dest = type_find_entry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;
	}
	free(table->entries);
	table->entries = entries;
	table->capacity = capacity;
}

bool type_table_get(const ObjectTypeTable *table, const ObjectString *key, ObjectTypeRecord **value)
{
	if (!table) {
		return false;
	}
	if (table->count == 0)
		return false;

	const TypeEntry *entry = type_find_entry(table->entries, table->capacity, key);
	if (entry->key == NULL)
		return false;
	*value = entry->value;
	return true;
}

bool type_table_delete(const ObjectTypeTable *table, const ObjectString *key)
{
	if (table->count == 0)
		return false;

	TypeEntry *entry = type_find_entry(table->entries, table->capacity, key);
	if (entry->key == NULL)
		return false;

	entry->key = NULL;
	entry->value = NULL;
	return true;
}

/**
 * Add all elements of `from` into `to`
 * @param from must be gc rooted
 * @param to must be gc rooted
 */
void type_table_add_all(const ObjectTypeTable *from, ObjectTypeTable *to)
{
	for (int i = 0; i < from->capacity; i++) {
		const TypeEntry *entry = &from->entries[i];
		if (entry->key != NULL) {
			type_table_set(to, entry->key, entry->value);
		}
	}
}

/**
 * Set a type in the type table
 * @return true if new type was set false otherwise
 */
bool type_table_set(ObjectTypeTable *table, ObjectString *key, ObjectTypeRecord *value)
{
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		const int capacity = TYPE_GROW_CAPACITY(table->capacity);
		type_table_adjust_capacity(table, capacity);
	}

	TypeEntry *entry = type_find_entry(table->entries, table->capacity, key);
	const bool isNewKey = entry->key == NULL;

	if (isNewKey && entry->value == NULL) {
		table->count++;
	}

	entry->key = key;
	entry->value = value;
	return isNewKey;
}
