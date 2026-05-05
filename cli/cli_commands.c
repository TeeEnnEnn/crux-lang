#include "cli_commands.h"
#include "cli_utils.h"
#include "file_handler.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include "linenoise.h"
#endif

// Recursive helper to install dependencies
static void process_dependencies(const char* base_path) {
    char manifest_path[2048];
#ifdef _WIN32
    snprintf(manifest_path, sizeof(manifest_path), "%s\\crux.json", base_path);
#else
    snprintf(manifest_path, sizeof(manifest_path), "%s/crux.json", base_path);
#endif

    const FileResult fileResult = read_file(manifest_path);
    if (fileResult.error) {
        free_file_result(fileResult);
        return; // No manifest, nothing to install here
    }

    cJSON* json = cJSON_Parse(fileResult.content);
    if (!json) {
        fprintf(stderr, RED "Error: Failed to parse %s\n" RESET, manifest_path);
        free_file_result(fileResult);
        return;
    }

    cJSON* dependencies = cJSON_GetObjectItemCaseSensitive(json, "dependencies");
    if (!cJSON_IsObject(dependencies) || cJSON_GetArraySize(dependencies) == 0) {
        cJSON_Delete(json);
        free_file_result(fileResult);
        return;
    }

    // Create local crux_modules for this package
    char modules_dir[2048];
#ifdef _WIN32
    snprintf(modules_dir, sizeof(modules_dir), "%s\\crux_modules", base_path);
#else
    snprintf(modules_dir, sizeof(modules_dir), "%s/crux_modules", base_path);
#endif
    ensure_dir_exists(modules_dir);

    cJSON* dep = NULL;
    cJSON_ArrayForEach(dep, dependencies) {
        const char* name = dep->string;
        const char* url = cJSON_GetStringValue(dep);

        if (name && url) {
            char pkg_path[2048];
#ifdef _WIN32
            snprintf(pkg_path, sizeof(pkg_path), "%s\\%s", modules_dir, name);
#else
            snprintf(pkg_path, sizeof(pkg_path), "%s/%s", modules_dir, name);
#endif

            printf("Installing pkg:%s from %s...\n", name, url);
            
            char command[4096];
#ifdef _WIN32
            snprintf(command, sizeof(command), "if not exist \"%s\" (git clone %s \"%s\") else (echo Package %s already exists.)", pkg_path, url, pkg_path, name);
#else
            snprintf(command, sizeof(command), "[ ! -d \"%s\" ] && git clone %s \"%s\" || echo \"Package %s already exists.\"", pkg_path, url, pkg_path, name);
#endif
            int res = system(command);
            if (res == 0) {
                // Recursively install dependencies for the new package
                process_dependencies(pkg_path);
            } else {
                fprintf(stderr, RED "Warning: Failed to install package '%s'.\n" RESET, name);
            }
        }
    }

    cJSON_Delete(json);
    free_file_result(fileResult);
}

int crux_cmd_repl(CruxVM *vm) {
    char *cruxDir = get_crux_dir();
    char *historyPath = NULL;
    if (cruxDir) {
        historyPath = combine_paths(cruxDir, "repl_history");
        free(cruxDir);
    }

#ifndef _WIN32
    const char *finalPath = historyPath ? historyPath : ".crux_history";
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

int crux_cmd_run(CruxVM *vm, const char *path) {
    const FileResult fileResult = read_file(path);
    if (fileResult.error) {
        fprintf(stderr, RED "Error: %s\n" RESET, fileResult.error);
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

int crux_cmd_init(const char* name) {
    const char* project_name = name ? name : "my_project";
    
    if (name) {
        printf("Creating directory '%s'...\n", name);
        if (!ensure_dir_exists(name)) {
            fprintf(stderr, RED "Error: Failed to create directory '%s'.\n" RESET, name);
            return 1;
        }
    }

    printf("Initializing new Crux project: %s...\n", project_name);

    char manifest_content[512];
    snprintf(manifest_content, sizeof(manifest_content), 
             "{\n"
             "  \"name\": \"%s\",\n"
             "  \"version\": \"0.1.0\",\n"
             "  \"main\": \"main.crux\",\n"
             "  \"dependencies\": {}\n"
             "}\n", project_name);

    char formatted_main[256];
    snprintf(formatted_main, sizeof(formatted_main), "println(\"Hello from %s!\");\n", project_name);

    const char* package_content = "// Entry point for re-exporting package symbols\n"
                                  "// pub use MyType from \"./internal_file.crux\";\n";

    char manifest_path[512];
    char main_path[512];
    char package_path[512];

    if (name) {
#ifdef _WIN32
        snprintf(manifest_path, sizeof(manifest_path), "%s\\crux.json", name);
        snprintf(main_path, sizeof(main_path), "%s\\main.crux", name);
        snprintf(package_path, sizeof(package_path), "%s\\pkg.crux", name);
#else
        snprintf(manifest_path, sizeof(manifest_path), "%s/crux.json", name);
        snprintf(main_path, sizeof(main_path), "%s/main.crux", name);
        snprintf(package_path, sizeof(package_path), "%s/pkg.crux", name);
#endif
    } else {
        strcpy(manifest_path, "crux.json");
        strcpy(main_path, "main.crux");
        strcpy(package_path, "pkg.crux");
    }

    FILE* f = fopen(manifest_path, "r");
    if (f) {
        fclose(f);
        fprintf(stderr, RED "Error: crux.json already exists at %s.\n" RESET, manifest_path);
        return 1;
    }

    f = fopen(manifest_path, "w");
    if (!f) {
        fprintf(stderr, RED "Error: Failed to create %s.\n" RESET, manifest_path);
        return 1;
    }
    fputs(manifest_content, f);
    fclose(f);

    f = fopen(main_path, "w");
    if (!f) {
        fprintf(stderr, RED "Error: Failed to create %s.\n" RESET, main_path);
        return 1;
    }
    fputs(formatted_main, f);
    fclose(f);

    f = fopen(package_path, "w");
    if (!f) {
        fprintf(stderr, RED "Error: Failed to create %s.\n" RESET, package_path);
        return 1;
    }
    fputs(package_content, f);
    fclose(f);

    printf("Success! Created project files.\n");
    return 0;
}

int crux_cmd_install(void) {
    // Start recursion from current directory
    process_dependencies(".");
    printf("Installation complete.\n");
    return 0;
}
