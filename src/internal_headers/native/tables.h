#ifndef TABLES_H
#define TABLES_H

#include "object.h"

CruxValue table_values_method(CruxVM *vm, const CruxValue *args);
CruxValue table_keys_method(CruxVM *vm, const CruxValue *args);
CruxValue table_pairs_method(CruxVM *vm, const CruxValue *args);
CruxValue table_remove_method(CruxVM *vm, const CruxValue *args);
CruxValue table_get_method(CruxVM *vm, const CruxValue *args);

CruxValue table_has_key_method(CruxVM *vm, const CruxValue *args);
CruxValue table_get_or_else_method(CruxVM *vm, const CruxValue *args);

#endif // TABLES_H
