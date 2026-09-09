#ifndef IO_H
#define IO_H

#include "value.h"

/*
 * io module — stream and terminal I/O
 *
 * Deals exclusively with reading from and writing to character streams
 * (stdin, stdout, stderr, or a named channel string).  Has no knowledge
 * of the filesystem; ObjectFile lives in fs.
 *
 * Channel strings accepted by the *_from / print_to variants:
 *   "stdin"  "stdout"  "stderr"
 */

/* ── Output ────────────────────────────────────────────────────────────────── */

/* print(value)
 * Writes a string representation of <value> to stdout. No newline. */
CruxValue io_print_function(CruxVM *vm, const CruxValue *args);

/* println(value)
 * Writes a string representation of <value> to stdout followed by '\n'. */
CruxValue io_println_function(CruxVM *vm, const CruxValue *args);

/* print_to(channel: string, value)  -> Result<nil>
 * Writes a string representation of <value> to the named channel.
 * Returns an error if the channel string is not recognised. */
CruxValue io_print_to_function(CruxVM *vm, const CruxValue *args);

/* println_to(channel: string, value)  -> Result<nil>
 * Same as print_to but appends '\n'. */
CruxValue io_println_to_function(CruxVM *vm, const CruxValue *args);

/* ── Input — stdin ─────────────────────────────────────────────────────────── */

/* scan()  -> Result<string>
 * Reads exactly one character from stdin and discards the rest of the line. */
CruxValue io_scan_function(CruxVM *vm, const CruxValue *args);

/* scanln()  -> Result<string>
 * Reads from stdin up to (and discarding) the next '\n'.
 * Returns the line without the newline character. */
CruxValue io_scanln_function(CruxVM *vm, const CruxValue *args);

/* nscan(n: int)  -> Result<string>
 * Reads up to <n> characters from stdin, stopping early on '\n'.
 * Discards any remaining characters up to '\n' when the limit is hit. */
CruxValue io_nscan_function(CruxVM *vm, const CruxValue *args);

/* ── Input — named channel ─────────────────────────────────────────────────── */

/* scan_from(channel: string)  -> Result<string>
 * Reads exactly one character from the named channel,
 * discarding the rest of the line. */
CruxValue io_scan_from_function(CruxVM *vm, const CruxValue *args);

/* scanln_from(channel: string)  -> Result<string>
 * Reads from the named channel up to (and discarding) the next '\n'. */
CruxValue io_scanln_from_function(CruxVM *vm, const CruxValue *args);

/* nscan_from(channel: string, n: int)  -> Result<string>
 * Reads up to <n> characters from the named channel,
 * stopping early on '\n'. */
CruxValue io_nscan_from_function(CruxVM *vm, const CruxValue *args);

#endif // IO_H
