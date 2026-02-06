#ifndef AFILE_H
#define AFILE_H

#include <errno.h> /* Since caller will check errors like ENOENT */

#include "arena.h"
#include "str.h"

/*
 * afile - Arena-based file operations
 *
 * All file contents are allocated from the provided arena.
 * Files are read entirely into memory (suitable for text files, configs).
 * Check .error field for errno on failure.
 */

/* Result of file read operation */
struct afile_result {
	struct str content; /* File contents (NUL-terminated) */
	int error;	    /* 0 on success, errno on failure */
};

/*
 * Read entire file into arena.
 * On success: .content has file data (NUL-terminated), .error is 0.
 * On failure: .content is STR_EMPTY, .error is errno.
 */
struct afile_result afile_read(struct arena *a, const char *path);

/* Like afile_read but accepts str path (converts internally). */
struct afile_result afile_read_str(struct arena *a, struct str path);

/* Result of reading file as lines */
struct afile_lines {
	struct str *lines; /* Array of line strings (views into arena) */
	int count;	   /* Number of lines */
	int error;	   /* 0 on success, errno on failure */
};

/*
 * Read file and split into array of lines.
 * Splits on '\n'. Lines do not include the newline character.
 * On failure: .lines is NULL, .count is 0, .error is errno.
 */
struct afile_lines afile_read_lines(struct arena *a, const char *path);

/* === Utility Functions (no arena allocation) === */

/* Check if file exists. Returns 1 if exists, 0 otherwise. */
int afile_exists(const char *path);

/* Get file size in bytes. Returns -1 on error (check errno). */
long afile_size(const char *path);

#endif
