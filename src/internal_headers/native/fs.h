#ifndef FS_H
#define FS_H

#include "value.h"

/*
 * fs module — filesystem operations
 *
 * All functions that touch files on disk live here.  The ObjectFile handle
 * (defined in object.h) is the currency for the method-style operations.
 *
 * Conventions:
 *   - Every function that can fail returns Result<T>.
 *   - Methods take the ObjectFile as args[0] (the receiver).
 *   - Paths are always ObjectString values.
 *   - Byte counts and offsets are int (int32_t in CruxValue terms).
 *   - "whence" for seek is a string: "start" | "current" | "end"
 */

/* ── File handle construction ──────────────────────────────────────────────── */

/* open(path: string, mode: string)  -> Result<File>
 * Opens the file at <path> with the given <mode> string ("r", "w", "a",
 * "r+", "rb", etc.).  The path is resolved relative to the current module. */
CruxValue fs_open_function(CruxVM *vm, const CruxValue *args);

/* ── Methods on File handles ───────────────────────────────────────────────── */

/* close()  -> Result<nil>
 * Flushes and closes the file handle.  Subsequent calls return an error. */
CruxValue fs_close_method(CruxVM *vm, const CruxValue *args);

/* flush()  -> Result<nil>
 * Flushes any buffered writes to disk without closing the handle. */
CruxValue fs_flush_method(CruxVM *vm, const CruxValue *args);

/* read(n: int)  -> Result<string>
 * Reads up to <n> bytes and returns them as a string.
 * Returns an empty string when at EOF. */
CruxValue fs_read_method(CruxVM *vm, const CruxValue *args);

/* readln()  -> Result<string>
 * Reads from the current position up to the next '\n' (exclusive).
 * Returns an empty string when at EOF. */
CruxValue fs_readln_method(CruxVM *vm, const CruxValue *args);

/* read_all()  -> Result<string>
 * Reads the entire remaining content of the file from the current position. */
CruxValue fs_read_all_method(CruxVM *vm, const CruxValue *args);

/* read_lines()  -> Result<Array<string>>
 * Reads all remaining lines into an Array.  Newline characters are stripped. */
CruxValue fs_read_lines_method(CruxVM *vm, const CruxValue *args);

/* write(content: string)  -> Result<nil>
 * Writes <content> to the file at the current position. */
CruxValue fs_write_method(CruxVM *vm, const CruxValue *args);

/* writeln(content: string)  -> Result<nil>
 * Writes <content> followed by '\n' to the file. */
CruxValue fs_writeln_method(CruxVM *vm, const CruxValue *args);

/* seek(offset: int, whence: string)  -> Result<nil>
 * Moves the file position.
 * <whence> must be one of: "start" | "current" | "end" */
CruxValue fs_seek_method(CruxVM *vm, const CruxValue *args);

/* tell()  -> Result<int>
 * Returns the current byte offset within the file. */
CruxValue fs_tell_method(CruxVM *vm, const CruxValue *args);

/* is_open()  -> bool   (infallible)
 * Returns true if the file handle is currently open. */
CruxValue fs_is_open_method(CruxVM *vm, const CruxValue *args);

/* ── Filesystem queries (path-based, no handle required) ───────────────────── */

/* exists(path: string)  -> bool   (infallible)
 * Returns true if a file or directory exists at <path>. */
CruxValue fs_exists_function(CruxVM *vm, const CruxValue *args);

/* is_file(path: string)  -> bool   (infallible)
 * Returns true if <path> exists and is a regular file. */
CruxValue fs_is_file_function(CruxVM *vm, const CruxValue *args);

/* is_dir(path: string)  -> bool   (infallible)
 * Returns true if <path> exists and is a directory. */
CruxValue fs_is_dir_function(CruxVM *vm, const CruxValue *args);

/* file_size(path: string)  -> Result<int>
 * Returns the size of the file in bytes. */
CruxValue fs_file_size_function(CruxVM *vm, const CruxValue *args);

/* ── Filesystem mutations (path-based) ─────────────────────────────────────── */

/* remove(path: string)  -> Result<nil>
 * Deletes the file at <path>. Returns an error if it does not exist
 * or is a directory. */
CruxValue fs_remove_function(CruxVM *vm, const CruxValue *args);

/* rename(from: string, to: string)  -> Result<nil>
 * Moves / renames a file or directory. */
CruxValue fs_rename_function(CruxVM *vm, const CruxValue *args);

/* copy_file(from: string, to: string)  -> Result<nil>
 * Copies the contents of <from> to <to>, creating or truncating <to>. */
CruxValue fs_copy_file_function(CruxVM *vm, const CruxValue *args);

/* mkdir(path: string)  -> Result<nil>
 * Creates a single directory.  Fails if the parent does not exist. */
CruxValue fs_mkdir_function(CruxVM *vm, const CruxValue *args);

/* ── Convenience (one-shot, no handle) ─────────────────────────────────────── */

/* read_file(path: string)  -> Result<string>
 * Opens the file, reads its entire content, closes it, returns the string. */
CruxValue fs_read_file_function(CruxVM *vm, const CruxValue *args);

/* write_file(path: string, content: string)  -> Result<nil>
 * Creates or truncates the file and writes <content> to it. */
CruxValue fs_write_file_function(CruxVM *vm, const CruxValue *args);

/* append_file(path: string, content: string) -> Result<nil>
 * Opens the file in append mode and writes <content> to it. */
CruxValue fs_append_file_function(CruxVM *vm, const CruxValue *args);

/* remove_dir(path: string) -> Result<nil>
 * Removes an empty directory. */
CruxValue fs_remove_dir_function(CruxVM *vm, const CruxValue *args);

#endif // FS_H
