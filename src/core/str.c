#include <core/memory.h>
#include <core/str.h>
#include <stdlib.h>
#include <string.h>

// Helper macros - simple versions to stay portable
// For production, could use typeof() with GNU extensions
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

struct str
str_from_cstr(const char *cstr)
{
	if (cstr == NULL) {
		return STR_EMPTY;
	}
	return (struct str){.data = cstr, .len = (int)strlen(cstr)};
}

struct str
str_from_parts(const char *data, int len)
{
	if (data == NULL || len <= 0) {
		return STR_EMPTY;
	}
	return (struct str){.data = data, .len = len};
}

struct str
str_slice(struct str s, int start, int end)
{
	if (start < 0)
		start = 0;
	if (start >= s.len)
		return STR_EMPTY;

	end = MIN(end, s.len);

	if (start >= end)
		return STR_EMPTY;

	return (struct str){.data = s.data + start, .len = end - start};
}

struct str_char_opt
str_at(struct str s, int index)
{
	if (index < 0 || index >= s.len) {
		return (struct str_char_opt){.value = 0, .valid = false};
	}
	return (struct str_char_opt){.value = s.data[index], .valid = true};
}

bool
str_eq(struct str a, struct str b)
{
	if (a.len != b.len) {
		return false;
	}
	if (a.len == 0) {
		return true; // Both empty
	}
	return memcmp(a.data, b.data, (size_t)a.len) == 0;
}

int
str_cmp(struct str a, struct str b)
{
	int min_len = MIN(a.len, b.len);

	if (min_len > 0) {
		int result = memcmp(a.data, b.data, (size_t)min_len);
		if (result != 0) {
			return result;
		}
	}

	// If prefixes are equal, shorter string is "less"
	if (a.len < b.len)
		return -1;
	if (a.len > b.len)
		return 1;
	return 0;
}

bool
str_starts_with(struct str s, struct str prefix)
{
	if (prefix.len > s.len) {
		return false;
	}
	if (prefix.len == 0) {
		return true; // Empty prefix matches anything
	}
	return memcmp(s.data, prefix.data, (size_t)prefix.len) == 0;
}

bool
str_ends_with(struct str s, struct str suffix)
{
	if (suffix.len > s.len) {
		return false;
	}
	if (suffix.len == 0) {
		return true; // Empty suffix matches anything
	}
	return memcmp(s.data + (s.len - suffix.len),
		      suffix.data,
		      (size_t)suffix.len) == 0;
}

struct str_search_result
str_find(struct str s, struct str needle)
{
	if (needle.len == 0) {
		return (struct str_search_result){.index = 0, .found = true};
	}
	if (needle.len > s.len) {
		return (struct str_search_result){.index = 0, .found = false};
	}

	// Simple brute-force search (good enough for most use cases)
	int search_len = s.len - needle.len + 1;
	for (int i = 0; i < search_len; i++) {
		if (memcmp(s.data + i, needle.data, (size_t)needle.len) == 0) {
			return (struct str_search_result){.index = i, .found = true};
		}
	}

	return (struct str_search_result){.index = 0, .found = false};
}

struct str_search_result
str_rfind(struct str s, struct str needle)
{
	if (needle.len == 0) {
		return (struct str_search_result){.index = s.len, .found = true};
	}
	if (needle.len > s.len) {
		return (struct str_search_result){.index = 0, .found = false};
	}

	// Search backwards
	for (int i = s.len - needle.len; i >= 0; i--) {
		if (memcmp(s.data + i, needle.data, (size_t)needle.len) == 0) {
			return (struct str_search_result){.index = i,
						   .found = true};
		}
	}

	return (struct str_search_result){.index = 0, .found = false};
}

char *
str_to_cstr(struct str s)
{
	// Allocate buffer with space for null terminator
	char *buf = xmalloc((size_t)s.len + 1);

	if (s.len > 0) {
		memcpy(buf, s.data, (size_t)s.len);
	}
	buf[s.len] = '\0';

	return buf;
}
