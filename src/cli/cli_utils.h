#ifndef CRUX_CLI_UTILS_H
#define CRUX_CLI_UTILS_H

#include "crux.h"

// ANSI colors
#define CYAN "\x1b[36m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"
#define RED "\x1b[31m"

/**
 * @brief Default write callback for the CruxVM.
 * Outputs text to stdout.
 */
void cli_write_fn(CruxVM* vm, const char* text);

/**
 * @brief Default error callback for the CruxVM.
 * Outputs formatted error messages to stderr.
 */
void cli_error_fn(CruxVM* vm, CruxErrorType type, const char* module_name, int line_number, const char* text);

#endif // CRUX_CLI_UTILS_H
