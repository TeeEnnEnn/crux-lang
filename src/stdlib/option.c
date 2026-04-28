#include "stdlib/option.h"

#include "object.h"

/**
 * args: [Option]
 * Returns true if the option is Some
 *  -> Bool
 */
CruxValue option_is_some_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	return BOOL_VAL(AS_CRUX_OPTION(args[0])->is_some);
}

/**
 * args: [Option]
 * Returns true if the option is None
 *  -> Bool
 */
CruxValue option_is_none_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	return BOOL_VAL(!AS_CRUX_OPTION(args[0])->is_some);
}

/**
 * args: [Option]
 * Unwraps an option
 * Returns the value if Some, otherwise returns NIL
 *  -> Any | Nil
 */
CruxValue option_unwrap_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectOption *option = AS_CRUX_OPTION(args[0]);
	return option->is_some ? option->value : NIL_VAL;
}

/**
 * args: [Option, Any]
 * Unwraps an option
 * Returns the value if Some, otherwise returns the default value
 *  -> Any
 */
CruxValue option_unwrap_or_method(CruxVM *vm, const CruxValue *args)
{
	(void)vm;
	const ObjectOption *option = AS_CRUX_OPTION(args[0]);
	return option->is_some ? option->value : args[1];
}
