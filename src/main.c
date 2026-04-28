#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crux.h"
#include "file_handler.h"
#include "cJSON.h"
#ifndef _WIN32
#include "linenoise.h"
#endif

// ANSI colors for REPL
#define CYAN "\x1b[36m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

static void default_write(CruxVM* vm, const char* text) {
    (void)vm;
    printf("%s", text);
}

static void default_error(struct CruxVM * vm, CruxErrorType type, const char * module_name, int line_number, const char * text) {
    (void)vm;
    (void)type;
    (void)module_name;
    (void)line_number;
    fprintf(stderr, text);
}

static int repl(CruxVM *vm)
{
	char *cruxDir = get_crux_dir();
	char *historyPath = NULL;
	if (cruxDir) {
		historyPath = combine_paths(cruxDir, "repl_history");
		free(cruxDir);
	}

	const char *finalPath = historyPath ? historyPath : ".crux_history";

#ifndef _WIN32
	linenoiseHistoryLoad(finalPath);
	char *line;

	while ((line = linenoise(CYAN "> " RESET)) != NULL) {
		if (line[0] != '\0') {
			linenoiseHistoryAdd(line);
			linenoiseHistorySave(finalPath);
			CruxInterpretResult res = crux_interpret(vm, "repl", line);
			if (res == CRUX_INTERPRET_EXIT) {
				linenoiseFree(line);
				break;
			}
		}
		linenoiseFree(line);
	}
#else
	while (true) {
		char line[1024];
		printf(CYAN "> " RESET);
		printf(GREEN);
		fflush(stdout);
		if (!fgets(line, sizeof(line), stdin)) {
			printf(RESET "\n");
			break;
		}
		printf(RESET);
		CruxInterpretResult res = crux_interpret(vm, "repl", line);
        if (res == CRUX_INTERPRET_EXIT) break;
	}
#endif

	if (historyPath)
		free(historyPath);

	return crux_vm_get_exit_code(vm);
}

static int run_file(CruxVM *vm, const char *path)
{
	const FileResult fileResult = read_file(path);
	if (fileResult.error) {
		fprintf(stderr, "Error reading file: %s\n", fileResult.error);
		return 2;
	}
	const CruxInterpretResult interpretResult = crux_interpret(vm, path, fileResult.content);
	free(fileResult.content);

	if (interpretResult == CRUX_INTERPRET_COMPILE_PANIC)
		return 65;
	if (interpretResult == CRUX_INTERPRET_RUNTIME_PANIC)
		return 70;
	if (interpretResult == CRUX_INTERPRET_EXIT)
		return crux_vm_get_exit_code(vm);

	return 0;
}

int main(const int argc, const char *argv[])
{
    CruxConfiguration config;
    init_crux_configuration(&config);
    config.writeFn = default_write;
    config.errorFn = default_error;

	if (argc == 2 && strcmp(argv[1], "-V") != 0 && strcmp(argv[1], "--version") != 0) {
        config.scriptPath = argv[1];
    }

	CruxVM *vm = crux_vm_new(&config);
	if (vm == NULL) {
		return 1;
	}
	int exit_code = 0;

	if (argc == 1) {
		exit_code = repl(vm);
	} else if (argc == 2) {
		if (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0) {
            printf("Crux %s\n", Crux_VERSION_STRING);
            exit_code = 0;
		} else {
			exit_code = run_file(vm, argv[1]);
		}
	} else {
#ifdef _WIN32
		fprintf(stderr, "Usage: & .\\[crux.exe] [path]\n");
#else
		fprintf(stderr, "Usage: ./[crux] [path]\n");
#endif
		exit_code = 64;
	}

	crux_vm_free(vm);
	return exit_code;
}
