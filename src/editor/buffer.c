#include <editor/buffer.h>

#include <string.h>

#include <core/afile.h>
#include <core/arena.h>

#define INITIAL_LINE_CAP 256

void
buffer_init(struct buffer *buf)
{
	arena_init(&buf->arena);
	buf->text = STR_EMPTY;
	buf->lines = NULL;
	buf->line_count = 0;
	buf->line_cap = 0;
	buf->cursor_line = 0;
	buf->path[0] = '\0';
}

void
buffer_destroy(struct buffer *buf)
{
	arena_destroy(&buf->arena);
	buf->text = STR_EMPTY;
	buf->lines = NULL;
	buf->line_count = 0;
	buf->line_cap = 0;
	buf->cursor_line = 0;
	buf->path[0] = '\0';
}

bool
buffer_load(struct buffer *buf, const char *path)
{
	struct str full;
	int i, line_start;
	int newline_count;

	/* Clear previous content */
	arena_reset(
	    &buf->arena); // or arena_destroy+arena_init, but reset is better
	buf->text = STR_EMPTY;
	buf->lines = NULL;
	buf->line_count = 0;
	buf->line_cap = 0;
	buf->cursor_line = 0;

	struct afile_result file_content = afile_read(&buf->arena, path);
	if (file_content.error > 0)
		return false;

	buf->text = file_content.content;
	full = buf->text;

	newline_count = 0;
	for (i = 0; i < full.len; i++)
		if (full.data[i] == '\n')
			newline_count++;

	buf->line_cap = newline_count + 1;

	/* allocate from arena, not heap */
	buf->lines = arena_array(&buf->arena, struct str, buf->line_cap);

	line_start = 0;
	buf->line_count = 0;
	for (i = 0; i <= full.len; i++) {
		if (i == full.len || full.data[i] == '\n') {
			buf->lines[buf->line_count++] = str_from_parts(
			    full.data + line_start, i - line_start);
			line_start = i + 1;
		}
	}

	strncpy(buf->path, path, BUFFER_PATH_MAX - 1);
	buf->path[BUFFER_PATH_MAX - 1] = '\0';
	return true;
}

struct str
buffer_get_text(struct buffer *buf)
{
	return buf->text;
}

struct str
buffer_get_line(struct buffer *buf, int line_num)
{
	if (line_num < 0 || line_num >= buf->line_count)
		return STR_EMPTY;
	return buf->lines[line_num];
}

struct str
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
