#ifndef CRUX_CLI_COMMANDS_H
#define CRUX_CLI_COMMANDS_H

#include "crux.h"

/**
 * @brief Starts the interactive REPL.
 */
int crux_cmd_repl(CruxVM *vm);

/**
 * @brief Runs a specific Crux script.
 */
int crux_cmd_run(CruxVM *vm, const char *path);

/**
 * @brief Initializes a new Crux project.
 */
int crux_cmd_init(const char *name);

/**
 * @brief Installs project dependencies from crux.json.
 */
int crux_cmd_install(void);

#endif // CRUX_CLI_COMMANDS_H
