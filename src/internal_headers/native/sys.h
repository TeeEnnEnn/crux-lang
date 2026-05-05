#ifndef SYS_H
#define SYS_H

#include "object.h"

CruxValue args_function(CruxVM *vm, const CruxValue *args);
CruxValue platform_function(CruxVM *vm, const CruxValue *args);
CruxValue arch_function(CruxVM *vm, const CruxValue *args);
CruxValue pid_function(CruxVM *vm, const CruxValue *args);
CruxValue get_env_function(CruxVM *vm, const CruxValue *args);
CruxValue exit_function(CruxVM *vm, const CruxValue *args);

#endif // SYS_H
