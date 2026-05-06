#include "cli_utils.h"
#include <stdio.h>

void cli_write_fn(CruxVM* vm, const char* text) {
    (void)vm;
    printf("%s", text);
}

void cli_error_fn(CruxVM* vm, CruxErrorType type, const char* module_name, int line_number, const char* text) {
    (void)vm;
    (void)type;
    (void)module_name;
    (void)line_number;
    fprintf(stderr, "%s", text);
}
