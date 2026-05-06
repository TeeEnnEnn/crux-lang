#include "native/range.h"
#include "object/object.h"
#include "panic.h"
#include "vm.h"

static bool range_is_empty(const ObjectRange *range)
{
	return range_len(range) <= 0;
}

/**
 * Creates a new range object.
 * arg0 -> start of the range
 * arg1 -> step of the range
 * arg2 -> end of the range
 * Return: Result<Range>
 */
CruxValue new_range_function(CruxVM *vm, const CruxValue *args)
{
	int32_t start = AS_INT(args[0]);
	int32_t step = AS_INT(args[1]);
	int32_t end = AS_INT(args[2]);

	const char *error_message = NULL;
	if (!validate_range_values(start, step, end, &error_message)) {
		return MAKE_GC_SAFE_ERROR(vm, error_message, VALUE);
	}

	ObjectRange *range = new_range(vm, start, end, step);
	push(vm->current_module_record, OBJECT_VAL(range));
	ObjectResult *result = new_ok_result(vm, OBJECT_VAL(range));
	pop(vm->current_module_record);
	return OBJECT_VAL(result);
}

/**
 * Checks if a value is contained within a range.
 * arg0 -> range
 * arg1 -> value
 * Return: Bool
 */
CruxValue contains_range_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectRange *range = AS_CRUX_RANGE(args[0]);
	int32_t value = AS_INT(args[1]);
	return BOOL_VAL(range_contains(range, value));
}

/**
 * Converts a range to an array.
 * arg0 -> range
 * Return: Array
 */
CruxValue to_array_range_method(CruxVM *vm, const CruxValue *args)
{
	const ObjectRange *range = AS_CRUX_RANGE(args[0]);
	int32_t len = range_len(range);

	ObjectArray *array = new_array(vm, len);
	push(vm->current_module_record, OBJECT_VAL(array));

	int32_t i = range->start;
	for (int32_t n = 0; n < len; n++, i += range->step) {
		if (!array_add_back(vm, array, INT_VAL(i))) {
			pop(vm->current_module_record);
			return MAKE_GC_SAFE_ERROR(vm, "Failed to add element to array.", VALUE);
		}
	}
	ObjectResult *result = new_ok_result(vm, OBJECT_VAL(array));
	pop(vm->current_module_record);
	return OBJECT_VAL(result);
}

/**
 * Returns the start value of a range.
 * arg0 -> range
 * Return: Int
 */
CruxValue start_range_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectRange *range = AS_CRUX_RANGE(args[0]);
	return INT_VAL(range->start);
}

/**
 * Returns the end value of a range.
 * arg0 -> range
 * Return: Int
 */
CruxValue end_range_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectRange *range = AS_CRUX_RANGE(args[0]);
	return INT_VAL(range->end);
}

/**
 * Returns the step value of a range.
 * arg0 -> range
 * Return: Int
 */
CruxValue step_range_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectRange *range = AS_CRUX_RANGE(args[0]);
	return INT_VAL(range->step);
}

/**
 * Returns true if the range is empty.
 * arg0 -> range
 * Return: Bool
 */
CruxValue is_empty_range_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectRange *range = AS_CRUX_RANGE(args[0]);
	return BOOL_VAL(range_is_empty(range));
}

/**
 * Returns a reversed range.
 * arg0 -> range
 * Return: Range
 */
CruxValue reversed_range_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectRange *range = AS_CRUX_RANGE(args[0]);
	return OBJECT_VAL(new_range(vm, range->end - range->step, range->start - range->step, -range->step));
}
