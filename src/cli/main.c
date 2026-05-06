#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crux.h"
#include "file_handler.h"
#include "cJSON.h"
#include "cli_commands.h"
#include "cli_utils.h"

static void print_usage(void) {
    fprintf(stderr, "Crux - A gradually typed, interpreted language.\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  crux [path]           - Run a specific script\n");
    fprintf(stderr, "  crux run [path]       - Run a script (defaults to manifest 'main')\n");
    fprintf(stderr, "  crux init [name]      - Initialize a new project\n");
    fprintf(stderr, "  crux install          - Install dependencies from crux.json\n");
    fprintf(stderr, "  crux -V, --version    - Show version\n");
}

int main(const int argc, const char *argv[])
{
    // 1. Handle non-VM commands
    if (argc > 1) {
        if (strcmp(argv[1], "init") == 0) {
            return crux_cmd_init(argc > 2 ? argv[2] : NULL);
        }
        if (strcmp(argv[1], "install") == 0) {
            return crux_cmd_install();
        }
        if (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0) {
            printf("Crux %s\n", CRUX_VERSION_STRING);
            return 0;
        }
    }

    // 2. Determine script to run
    const char* scriptPath = NULL;
    char* allocatedPath = NULL;

    if (argc == 1) {
        // REPL mode
    } else if (argc == 2 && argv[1][0] != '-') {
        // crux <file>
        scriptPath = argv[1];
    } else if (strcmp(argv[1], "run") == 0) {
        if (argc == 3) {
            // crux run <file>
            scriptPath = argv[2];
        } else if (argc == 2) {
            // crux run (look for manifest)
            const FileResult fr = read_file("crux.json");
            if (!fr.error) {
                cJSON* json = cJSON_Parse(fr.content);
                if (json) {
                    cJSON* main_field = cJSON_GetObjectItemCaseSensitive(json, "main");
                    if (cJSON_IsString(main_field) && (main_field->valuestring != NULL)) {
                        allocatedPath = strdup(main_field->valuestring);
                        scriptPath = allocatedPath;
                    }
                    cJSON_Delete(json);
                }
                free_file_result(fr);
            }
            if (!scriptPath) scriptPath = "main.crux"; // Fallback
        }
    } else {
        print_usage();
        return 64;
    }

    // 3. Initialize VM and Execute
    CruxConfiguration config;
    init_crux_configuration(&config);
    config.writeFn = cli_write_fn;
    config.errorFn = cli_error_fn;
    config.scriptPath = scriptPath;

	CruxVM *vm = crux_vm_new(&config);
	if (vm == NULL) return 1;

	int exit_code = 0;
	if (scriptPath == NULL) {
		exit_code = crux_cmd_repl(vm);
	} else {
		exit_code = crux_cmd_run(vm, scriptPath);
	}

	crux_vm_free(vm);
    if (allocatedPath) free(allocatedPath);
	return exit_code;
}
