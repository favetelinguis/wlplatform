#define _POSIX_C_SOURCE 200809L

#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_LINE_CAP 256

void
buffer_init(struct buffer *buf)
{
	buf->text = NULL;
	buf->lines = NULL;
	buf->line_count = 0;
	buf->line_cap = 0;
	buf->cursor_line = 0;
	buf->path[0] = '\0';
}

void
buffer_destroy(struct buffer *buf)
{

	if (buf->text)
		str_buf_free(buf->text);
	free(buf->lines);
	buffer_init(buf); /* Reset to clean state */
}

bool
buffer_load(struct buffer *buf, const char *path)
{
	FILE *fp;
	long file_size;
	char *raw;
	str full;
	size_t i, line_start;
	int newline_count;

	/* Clear any existing content */
	buffer_destroy(buf);

	/* Open and get file size */
	fp = fopen(path, "r");
	if (!fp)
		return false;
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Create str_buf with exact capacity */
	buf->text = str_buf_with_capacity((size_t)file_size);
	if (!buf->text) {
		fclose(fp);
		return false;
	}

	/* Read file directly into temporary buffer, then append */
	raw = malloc((size_t)file_size);
	if (!raw) {
		fclose(fp);
		buffer_destroy(buf);
		return false;
	}

	if (fread(raw, 1, (size_t)file_size, fp) != (size_t)file_size) {
		free(raw);
		fclose(fp);
		buffer_destroy(buf);
		return false;
	}
	fclose(fp);

	/* Append to str_buf (maintains null terminator) */
	str_buf_append(buf->text, str_from_parts(raw, (size_t)file_size));
	free(raw);

	/* Get view for scanning */
	full = str_buf_view(buf->text);

	/* Count newlines to allocate line array */
	newline_count = 0;
	for (i = 0; i < full.len; i++) {
		if (full.data[i] == '\n')
			newline_count++;
	}

	buf->line_cap = newline_count + 1;
	buf->lines = malloc(buf->line_cap * sizeof(str));
	if (!buf->lines) {
		buffer_destroy(buf);
		return false;
	}

	/* Build line index as str views */
	line_start = 0;
	buf->line_count = 0;
	for (i = 0; i <= full.len; i++) {
		if (i == full.len || full.data[i] == '\n') {
			buf->lines[buf->line_count] = str_from_parts(
			    full.data + line_start, i - line_start);
			buf->line_count++;
			line_start = i + 1;
		}
	}
	strncpy(buf->path, path, BUFFER_PATH_MAX - 1);
	buf->path[BUFFER_PATH_MAX - 1] = '\0';
	return true;
}

str
buffer_get_text(struct buffer *buf)
{
	if (!buf->text)
		return STR_EMPTY;
	return str_buf_view(buf->text);
}

str
buffer_get_line(struct buffer *buf, int line_num)
{
	if (line_num < 0 || line_num >= buf->line_count)
		return STR_EMPTY;
	return buf->lines[line_num];
}

str
buffer_get_current_line(struct buffer *buf)
{
	return buffer_get_line(buf, buf->cursor_line);
}

const char *
buffer_get_text_ctr(struct buffer *buf)
{
	if (!buf->text)
		return "";
	return str_buf_cstr(buf->text);
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
