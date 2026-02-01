#define _POSIX_C_SOURCE 200809L

#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_LINE_CAP 256

void
buffer_init(struct buffer *buf)
{
	memset(buf, 0, sizeof(*buf));
}

void
buffer_destroy(struct buffer *buf)
{
	int i;

	for (i = 0; i < buf->line_count; i++)
		free(buf->lines[i]);
	free(buf->lines);
	buffer_init(buf); /* Reset to clean state */
}

/*
 * Double array capacity when full.
 * Class growth strategy: amortized O(1) insertion.
 */
static bool
grow_lines(struct buffer *buf)
{
	int new_cap;
	char **new_lines;

	new_cap = buf->line_cap ? buf->line_cap * 2 : INITIAL_LINE_CAP;
	new_lines = realloc(buf->lines, new_cap * sizeof(char *));
	if (!new_lines)
		return false;

	buf->lines = new_lines;
	buf->line_cap = new_cap;
	return true;
}

bool
buffer_load(struct buffer *buf, const char *path)
{
	FILE *fp;
	char line_buf[BUFFER_LINE_MAX];
	size_t len;

	/* Clear any existing content */
	buffer_destroy(buf);

	fp = fopen(path, "r");
	if (!fp)
		return false;

	/* Store path */
	strncpy(buf->path, path, BUFFER_PATH_MAX - 1);
	buf->path[BUFFER_PATH_MAX - 1] = '\0';

	/* Read lines */
	while (fgets(line_buf, sizeof(line_buf), fp)) {
		/* Strip trailing newline */
		len = strlen(line_buf);
		if (len > 0 && line_buf[len - 1] == '\n')
			line_buf[--len] = '\0';

		/* Grow array if needed */
		if (buf->line_count >= buf->line_cap) {
			if (!grow_lines(buf)) {
				fclose(fp);
				buffer_destroy(buf);
				return false;
			}
		}

		/* Duplicate line into heap */
		buf->lines[buf->line_count] = strdup(line_buf);
		if (!buf->lines[buf->line_count]) {
			fclose(fp);
			buffer_destroy(buf);
			return false;
		}
		buf->line_count++;
	}
	fclose(fp);
	return true;
}

const char *
buffer_get_line(struct buffer *buf, int line_num)
{
	if (line_num < 0 || line_num >= buf->line_count)
		return NULL;
	return buf->lines[line_num];
}

const char *
buffer_get_current_line(struct buffer *buf)
{
	return buffer_get_line(buf, buf->cursor_line);
}

void
buffer_move_down(struct buffer *buf, int n)
{
	buf->cursor_line += n;

	/* Clamp to valid range */
	if (buf->cursor_line >= buf->line_count)
		buf->cursor_line = buf->line_count - 1;
	if (buf->cursor_line < 0)
		buf->cursor_line = 0;
}

void
buffer_move_up(struct buffer *buf, int n)
{
	buffer_move_down(buf, -n);
}
