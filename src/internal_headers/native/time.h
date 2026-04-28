#ifndef TIME_H
#define TIME_H

#include "object.h"
#include "value.h"

// Current time functions
CruxValue time_seconds_function_(CruxVM *vm, const CruxValue *args);
CruxValue time_milliseconds_function_(CruxVM *vm, const CruxValue *args);

// Sleep functions
CruxValue sleep_seconds_function(CruxVM *vm, const CruxValue *args);
CruxValue sleep_milliseconds_function(CruxVM *vm,
					  const CruxValue *args);

// Date/Time functions
CruxValue year_function_(CruxVM *vm, const CruxValue *args);
CruxValue month_function_(CruxVM *vm, const CruxValue *args);
CruxValue day_function_(CruxVM *vm, const CruxValue *args);
CruxValue hour_function_(CruxVM *vm, const CruxValue *args);
CruxValue minute_function_(CruxVM *vm, const CruxValue *args);
CruxValue second_function_(CruxVM *vm, const CruxValue *args);
CruxValue weekday_function_(CruxVM *vm, const CruxValue *args);
CruxValue day_of_year_function_(CruxVM *vm, const CruxValue *args);

#endif // TIME_H
