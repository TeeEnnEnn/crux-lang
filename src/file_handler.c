#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "file_handler.h"
/**
 * @brief Extracts the directory name from a file path,
 *
 * Parses a file path and returns the directory portion.
 * Handles platform-specific path separators and edge cases.
 *
 * @param path The file path to process
 * @return Dynamically allocated string containing the directory name (caller
 * must free)
 */
static char *dirName(const char *path)
{
	if (path == NULL) {
		return NULL;
	}

	char *pathCopy = strdup(path);
	if (pathCopy == NULL) {
		return NULL;
	}

	size_t pathLen = strlen(pathCopy);
	if (pathLen == 0) {
		free(pathCopy);
		return strdup(".");
	}

	while (pathLen > 1 && (pathCopy[pathLen - 1] == '/' || pathCopy[pathLen - 1] == '\\')) {
		pathCopy[pathLen--] = '\0';
	}

	char *lastSlash = strrchr(pathCopy, '/');
#ifdef _WIN32
	char *lastBackslash = strrchr(pathCopy, '\\');
	if (lastBackslash > lastSlash) {
		lastSlash = lastBackslash;
	}
#endif
	if (lastSlash == NULL) {
		free(pathCopy);
		return strdup(".");
	}

	if (lastSlash == pathCopy) {
#ifdef _WIN32
		char *result;
		if (pathCopy[1] == ':') {
			result = strdup(pathCopy);
		} else {
			result = strdup("\\");
		}
#else
		char *result = strdup("/");
#endif
		free(pathCopy);
		return result;
	}
#ifdef _WIN32
	if (lastSlash == pathCopy + 2 && pathCopy[1] == ':') {
		char *result = malloc(4);
		if (result == NULL) {
			free(pathCopy);
			return NULL;
		}
		result[0] = pathCopy[0];
		result[1] = ':';
		result[2] = '\\';
		result[3] = '\0';
		free(pathCopy);
		return result;
	}
#endif
	*lastSlash = '\0';
	char *result = strdup(pathCopy);
	free(pathCopy);
	return result;
}

/**
 * @brief Gets the directory component from a path,
 *
 * Creates a copy of the input path and uses dirName() to extract
 * the directory component. Handles memory management by making
 * a separate copy of the result.
 *
 * @param path The file path to process
 * @return Dynamically allocated string containing the directory (caller must
 * free)
 */
static char *get_directory_from_path(const char *path)
{
	if (path == NULL) {
		return NULL;
	}

	char *pathCopy = strdup(path);
	if (pathCopy == NULL) {
		return NULL;
	}

	char *dir = dirName(pathCopy);
	if (dir == NULL) {
		free(pathCopy);
		return NULL;
	}

	char *result = strdup(dir);
	free(pathCopy);
	free(dir);
	return result;
}

char *combine_paths(const char *base, const char *relative)
{
	if (base == NULL || relative == NULL)
		return NULL;

	if (relative[0] == '/'
#ifdef _WIN32
		|| (strlen(relative) > 2 && relative[1] == ':')
#endif
	) {
		return strdup(relative);
	}

	const size_t base_len = strlen(base);
	const size_t relative_len = strlen(relative);
	const size_t total_len = base_len + 1 + relative_len + 1; // +1 : '/' +1 '\0'

	char *result = malloc(total_len);
	if (result == NULL)
		return NULL;

	strcpy(result, base);

	if (base_len > 0 && base[base_len - 1] != '/' && base[base_len - 1] != '\\') {
#ifdef _WIN32
		strcat(result, "\\");
#else
		strcat(result, "/");
#endif
	}

	strcat(result, relative);
	return result;
}

static bool is_valid_package_name(const char *name)
{
	if (name == NULL || *name == '\0')
		return false;
	for (const char *p = name; *p; p++) {
		if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-')
			return false;
	}
	return true;
}

static bool file_exists(const char *path)
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributesA(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
	struct stat buffer;
	return (stat(path, &buffer) == 0);
#endif
}

char *resolve_path(const char *base_path, const char *import_path)
{
	if (import_path == NULL)
		return NULL;

	// 1. Handle "std:X" (Package or File)
	if (strncmp(import_path, "std:", 4) == 0) {
		const char *module_name = import_path + 4;
		char *stdlib_path = getenv("CRUX_STDLIB");
		bool free_stdlib = false;

		if (stdlib_path == NULL) {
			stdlib_path = get_crux_dir();
			if (stdlib_path) {
				char *temp = combine_paths(stdlib_path, "stdlib");
				free(stdlib_path);
				stdlib_path = temp;
				free_stdlib = true;
			}
		}

		if (stdlib_path == NULL)
			return NULL;

		// Try directory package first: std/X/pkg.crux
		char dir_pkg_rel[256];
#ifdef _WIN32
		snprintf(dir_pkg_rel, sizeof(dir_pkg_rel), "%s\\pkg.crux", module_name);
#else
		snprintf(dir_pkg_rel, sizeof(dir_pkg_rel), "%s/pkg.crux", module_name);
#endif
		char *dir_resolved_temp = combine_paths(stdlib_path, dir_pkg_rel);

#ifdef _WIN32
		char *dir_resolved = malloc(MAX_PATH_LENGTH);
		if (_fullpath(dir_resolved, dir_resolved_temp, MAX_PATH_LENGTH)) {
			if (file_exists(dir_resolved)) {
				free(dir_resolved_temp);
				if (free_stdlib)
					free(stdlib_path);
				return dir_resolved;
			}
		}
		free(dir_resolved);
#else
		char dir_resolved[MAX_PATH_LENGTH];
		if (realpath(dir_resolved_temp, dir_resolved)) {
			if (file_exists(dir_resolved)) {
				free(dir_resolved_temp);
				if (free_stdlib)
					free(stdlib_path);
				return strdup(dir_resolved);
			}
		}
#endif
		free(dir_resolved_temp);

		// Fallback to single file: std/X.crux
		char filename[256];
		snprintf(filename, sizeof(filename), "%s.crux", module_name);
		char *resolved_temp = combine_paths(stdlib_path, filename);

#ifdef _WIN32
		char *resolved = malloc(MAX_PATH_LENGTH);
		if (_fullpath(resolved, resolved_temp, MAX_PATH_LENGTH)) {
			free(resolved_temp);
			if (free_stdlib)
				free(stdlib_path);
			return resolved;
		}
		free(resolved);
#else
		char resolved[MAX_PATH_LENGTH];
		if (realpath(resolved_temp, resolved)) {
			free(resolved_temp);
			if (free_stdlib)
				free(stdlib_path);
			return strdup(resolved);
		}
#endif
		free(resolved_temp);

		if (free_stdlib)
			free(stdlib_path);
		return NULL;
	}

	// 2. Handle "pkg:X" (Upward Traversal with Manifest Boundary)
	if (strncmp(import_path, "pkg:", 4) == 0) {
		const char *package_name = import_path + 4;

		if (!is_valid_package_name(package_name)) {
			return NULL;
		}

		char *current_search_dir = base_path ? get_directory_from_path(base_path) : strdup(".");

		while (current_search_dir != NULL) {
			char rel_pkg_path[512];
#ifdef _WIN32
			snprintf(rel_pkg_path, sizeof(rel_pkg_path), "crux_modules\\%s\\pkg.crux", package_name);
#else
			snprintf(rel_pkg_path, sizeof(rel_pkg_path), "crux_modules/%s/pkg.crux", package_name);
#endif
			char *full_pkg_path = combine_paths(current_search_dir, rel_pkg_path);

			if (file_exists(full_pkg_path)) {
				// Found it!
#ifdef _WIN32
				char *final_path = malloc(MAX_PATH_LENGTH);
				if (_fullpath(final_path, full_pkg_path, MAX_PATH_LENGTH)) {
					free(full_pkg_path);
					free(current_search_dir);
					return final_path;
				}
#else
				char final_path[MAX_PATH_LENGTH];
				if (realpath(full_pkg_path, final_path)) {
					free(full_pkg_path);
					free(current_search_dir);
					return strdup(final_path);
				}
#endif
			}
			free(full_pkg_path);

			// SECURITY: Check if this directory contains a manifest.
			// If it does, we have reached the project root and must stop searching upward.
			char *manifest_path = combine_paths(current_search_dir, "crux.json");
			bool is_root = file_exists(manifest_path);
			free(manifest_path);

			if (is_root) {
				break;
			}

			// Move up one directory
			char *parent = dirName(current_search_dir);
			if (parent == NULL || strcmp(parent, current_search_dir) == 0) {
				if (parent)
					free(parent);
				break;
			}
			free(current_search_dir);
			current_search_dir = parent;
		}

		if (current_search_dir)
			free(current_search_dir);
		return NULL; // Not found within project boundary
	}

	// 3. Handle normal relative/absolute paths
	if (base_path == NULL || import_path[0] == '/'
#ifdef _WIN32
		|| (strlen(import_path) > 2 && import_path[1] == ':')
#endif
	) {
#ifdef _WIN32
		char *resolved_path = malloc(MAX_PATH_LENGTH);
		if (_fullpath(resolved_path, import_path, MAX_PATH_LENGTH) == NULL) {
			free(resolved_path);
			return NULL;
		}
		return resolved_path;
#else
		char resolvedPath[MAX_PATH_LENGTH];
		if (realpath(import_path, resolvedPath) == NULL) {
			return strdup(import_path);
		}
		return strdup(resolvedPath);
#endif
	}

	char *baseDir = get_directory_from_path(base_path);
	if (baseDir == NULL)
		return NULL;

	char *combinedPath = combine_paths(baseDir, import_path);
	free(baseDir);
	if (combinedPath == NULL)
		return NULL;

#ifdef _WIN32
	char *resolvedPath = malloc(MAX_PATH_LENGTH);
	if (_fullpath(resolvedPath, combinedPath, MAX_PATH_LENGTH) == NULL) {
		free(combinedPath);
		free(resolvedPath);
		return NULL;
	}
	free(combinedPath);
	return resolvedPath;
#else
	char resolvedPath[MAX_PATH_LENGTH];
	if (realpath(combinedPath, resolvedPath) == NULL) {
		char *result = strdup(combinedPath);
		free(combinedPath);
		return result;
	}
	free(combinedPath);
	return strdup(resolvedPath);
#endif
}

FileResult read_file(const char *path)
{
	FileResult result = {NULL, NULL};
	FILE *file = fopen(path, "rb");

	if (file == NULL) {
		const size_t errorLen = strlen(path) + 32;
		result.error = (char *)malloc(errorLen);
		if (result.error != NULL) {
			snprintf(result.error, errorLen, "Could not open file \"%s\"", path);
		}
		return result;
	}

	fseek(file, 0L, SEEK_END);
	const size_t fileSize = ftell(file);
	rewind(file);

	result.content = (char *)malloc(fileSize + 1);
	if (result.content == NULL) {
		result.error = strdup("Not enough memory to read file");
		fclose(file);
		return result;
	}

	const size_t bytesRead = fread(result.content, 1, fileSize, file);
	if (bytesRead < fileSize) {
		free(result.content);
		result.content = NULL;
		result.error = strdup("Could not read file completely");
		fclose(file);
		return result;
	}

	result.content[bytesRead] = '\0';
	fclose(file);
	return result;
}

void free_file_result(const FileResult result)
{
	free(result.content);
	free(result.error);
}

bool ensure_dir_exists(const char *path)
{
	if (mkdir(path, 0700) == 0) {
		return true;
	}

	if (errno == EEXIST) {
		return true;
	}

	return false;
}

char *get_home_dir(void)
{
	char *home = getenv("HOME");
#ifdef _WIN32
	if (home == NULL) {
		home = getenv("USERPROFILE");
	}
#endif
	return home ? strdup(home) : NULL;
}

char *get_crux_dir(void)
{
	char *home = get_home_dir();
	if (home == NULL) {
		return NULL;
	}

	char *cruxDir = combine_paths(home, ".crux");
	free(home);

	if (cruxDir == NULL) {
		return NULL;
	}

	if (!ensure_dir_exists(cruxDir)) {
		free(cruxDir);
		return NULL;
	}

	return cruxDir;
}
