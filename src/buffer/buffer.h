#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>

#include "../string/str.h"
#include "../string/str_buf.h"

#define BUFFER_PATH_MAX 4096
#define BUFFER_LINE_MAX 4096 /* Max line length we handle */

struct buffer {
	char path[BUFFER_PATH_MAX]; /* Absolute path to file */

	str_buf *text;	/* Owned file content (contiguous, maintains null
			   termination) */
	str *lines;	/* Line index: array of str views into text */
	int line_count; /* Number of lines */
	int line_cap;	/* Allocated capacity */

	int cursor_line; /* Current line (0-indexed, shown in input) */
};

/*
 * Initialize buffer to empty state.
 * Must call befoer any other operation.
 */
void buffer_init(struct buffer *buf);

/*
 * Free all allocated memory.
 * Buffer is invalid after this call.
 */
void buffer_destroy(struct buffer *buf);

/*
 * Load file contents into buffer.
 * Returns true on success, false on error.
 *
 * Note: Replaces any existing content.
 */
bool buffer_load(struct buffer *buf, const char *path);

/* Get full text for parsing */
str buffer_get_text(struct buffer *buf);

/*
 * Get line at given index.
 * Return NULL if index out of bounds.
 */
str buffer_get_line(struct buffer *buf, int line_num);

/*
 * Get current line (at cursor_line)
 */
str buffer_get_current_line(struct buffer *buf);

/* For C string APIs (UI labels that need null-terminated) */
const char *buffer_get_text_cstr(struct buffer *buf);

/*
 * Move cursor down by n lines (scroll buffer up throgh input).
 * Clamps to valid range.
 */
void buffer_move_down(struct buffer *buf, int n);

/*
 * Mover cursor up by n lines (scroll buffer down through input).
 * Clamps to valid range.
 */
void buffer_move_up(struct buffer *buf, int n);

#endif /* BUFFER_H */
