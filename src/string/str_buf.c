#include "str_buf.h"
#include "arena.h"
#include "compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Initial capacity for new string buffers */
static const size_t STR_BUF_INITIAL_CAPACITY = 16;

/* Arena default block size */
static const size_t ARENA_DEFAULT_BLOCK_SIZE = 4096;

/* Internal structure - not visible to users */
struct str_buf {
	char *data;
	size_t len;
	size_t capacity;
	arena *arena;
};

/* Helper: grow capacity (similar to clox GROW_CAPACITY pattern) */
#define NEXT_CAPACITY(cap) ((cap) < 8 ? 8 : (cap) * 2)

str_buf *
str_buf_new(void)
{
	return str_buf_with_capacity(STR_BUF_INITIAL_CAPACITY);
}

str_buf *
str_buf_with_capacity(size_t capacity)
{
	str_buf *sb = malloc(sizeof(str_buf));
	if (sb == NULL) {
		return NULL;
	}

	/* Create arena for efficient allocations */
	sb->arena = arena_create_with_size(ARENA_DEFAULT_BLOCK_SIZE);
	if (sb->arena == NULL) {
		free(sb);
		return NULL;
	}

	/* Ensure minimum capacity */
	if (capacity < STR_BUF_INITIAL_CAPACITY) {
		capacity = STR_BUF_INITIAL_CAPACITY;
	}

	/* Allocate initial buffer (+1 for null terminator) */
	sb->data = arena_alloc_aligned(sb->arena, capacity + 1, ALIGNOF(char));
	if (sb->data == NULL) {
		arena_free(sb->arena);
		free(sb);
		return NULL;
	}

	sb->len = 0;
	sb->capacity = capacity;
	sb->data[0] = '\0';

	return sb;
}

void
str_buf_free(str_buf *sb)
{
	if (sb == NULL) {
		return;
	}

	if (sb->arena != NULL) {
		arena_free(sb->arena);
	}

	free(sb);
}

/* Internal helper: ensure capacity for additional bytes */
static void
ensure_capacity(str_buf *sb, size_t needed)
{
	if (sb->len + needed <= sb->capacity) {
		return;  /* Already have enough space */
	}

	/* Calculate new capacity - keep doubling until we have enough */
	size_t new_capacity = sb->capacity;
	while (new_capacity < sb->len + needed) {
		new_capacity = NEXT_CAPACITY(new_capacity);
	}

	/* Allocate new buffer from arena (+1 for null terminator) */
	char *new_data = arena_alloc_aligned(sb->arena, new_capacity + 1, ALIGNOF(char));
	if (new_data == NULL) {
		/* Allocation failed - keep old buffer */
		return;
	}

	/* Copy existing data to new buffer */
	if (sb->len > 0) {
		memcpy(new_data, sb->data, sb->len);
	}
	new_data[sb->len] = '\0';

	/* Update buffer (old buffer stays in arena, will be freed together) */
	sb->data = new_data;
	sb->capacity = new_capacity;
}

void
str_buf_push(str_buf *sb, char c)
{
	if (sb == NULL) {
		return;
	}

	ensure_capacity(sb, 1);

	sb->data[sb->len] = c;
	sb->len++;
	sb->data[sb->len] = '\0';
}

void
str_buf_append(str_buf *sb, str s)
{
	if (sb == NULL || s.len == 0) {
		return;
	}

	ensure_capacity(sb, s.len);

	memcpy(sb->data + sb->len, s.data, s.len);
	sb->len += s.len;
	sb->data[sb->len] = '\0';
}

void
str_buf_append_cstr(str_buf *sb, const char *cstr)
{
	if (sb == NULL || cstr == NULL) {
		return;
	}

	str_buf_append(sb, str_from_cstr(cstr));
}

void
str_buf_clear(str_buf *sb)
{
	if (sb == NULL) {
		return;
	}

	sb->len = 0;
	if (sb->data != NULL) {
		sb->data[0] = '\0';
	}
}

void
str_buf_reserve(str_buf *sb, size_t additional)
{
	if (sb == NULL) {
		return;
	}

	ensure_capacity(sb, additional);
}

size_t
str_buf_len(const str_buf *sb)
{
	return sb != NULL ? sb->len : 0;
}

size_t
str_buf_capacity(const str_buf *sb)
{
	return sb != NULL ? sb->capacity : 0;
}

str
str_buf_view(const str_buf *sb)
{
	if (sb == NULL || sb->data == NULL) {
		return STR_EMPTY;
	}

	return (str){.data = sb->data, .len = sb->len};
}

const char *
str_buf_cstr(const str_buf *sb)
{
	if (sb == NULL || sb->data == NULL) {
		return "";
	}

	return sb->data;
}
