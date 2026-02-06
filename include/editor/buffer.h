/* include/editor/buffer.h
 *
 * Text buffer with line indexing.
 * Layer 3 - depends on core/ only.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>

#include <core/arena.h>
#include <core/str.h>

#define BUFFER_PATH_MAX 512

struct buffer {
	struct arena arena;
	struct str text;      /* Full file content */
	struct str *lines;    /* Array of line views into text */
	int line_count;
	int line_cap;
	int cursor_line;
	char path[BUFFER_PATH_MAX];
};

void buffer_init(struct buffer *buf);
void buffer_destroy(struct buffer *buf);
bool buffer_load(struct buffer *buf, const char *path);

struct str buffer_get_text(struct buffer *buf);
struct str buffer_get_line(struct buffer *buf, int line_num);
struct str buffer_get_current_line(struct buffer *buf);

void buffer_move_down(struct buffer *buf, int n);
void buffer_move_up(struct buffer *buf, int n);

#endif /* BUFFER_H */
