#ifndef STR_H
#define STR_H

#include <stdbool.h>
#include <stddef.h>

struct str {
	const char *data; /* Non-owning pointer */
	int len;	  /* Length in bytes (not including null terminator) */
};

/* Constant for empty string */
#define STR_EMPTY ((struct str){.data = "", .len = 0})

/* Convenience macro for string literals using compound literal + designated
 * initializer */
#define STR_LIT(s) ((struct str){.data = (s), .len = sizeof(s) - 1})

/* === Construction === */
/* Pass str by value (it's just 2 words) */

/*
 * Create str from NUL-terminated C string.
 * Returns STR_EMPTY if cstr is NULL.
 */
struct str str_from_cstr(const char *cstr);

/* Create str from pointer and length. Does not copy data. */
struct str str_from_parts(const char *data, int len);

/*
 * Create substring view [start, end).
 * Clamps indices to valid range. Returns STR_EMPTY if start >= end.
 */
struct str str_slice(struct str s, int start, int end);

/* === Inspection === */

/* Get length of string */
static inline int
str_len(struct str s)
{
	return s.len;
}

/* Check if string is empty */
static inline bool
str_empty(struct str s)
{
	return s.len == 0;
}

/* Get pointer to data */
static inline const char *
str_data(struct str s)
{
	return s.data;
}

/* === Character Access === */

/* Optional character - use .valid to check before accessing .value */
struct str_char_opt {
	char value;
	bool valid;
};

/*
 * Get character at index with bounds checking.
 * Returns {char, true} if valid, {0, false} if out of bounds.
 */
struct str_char_opt str_at(struct str s, int index);

/* === Comparison === */

/* Check if two strings are byte-equal. */
bool str_eq(struct str a, struct str b);

/* Lexicographic comparison. Returns <0 if a<b, 0 if equal, >0 if a>b. */
int str_cmp(struct str a, struct str b);

/* Check if s starts with prefix. Empty prefix matches any string. */
bool str_starts_with(struct str s, struct str prefix);

/* Check if s ends with suffix. Empty suffix matches any string. */
bool str_ends_with(struct str s, struct str suffix);

/* === Search === */

/* Search result - use .found to check before accessing .index */
struct str_search_result {
	int index;
	bool found;
};

/*
 * Find first occurrence of needle in s.
 * Returns {index, true} if found, {0, false} if not found.
 * Empty needle matches at index 0.
 */
struct str_search_result str_find(struct str s, struct str needle);

/*
 * Find last occurrence of needle in s.
 * Returns {index, true} if found, {0, false} if not found.
 */
struct str_search_result str_rfind(struct str s, struct str needle);

/* === Conversion === */

/*
 * Convert to heap-allocated NUL-terminated C string.
 * Caller must free() the returned pointer. Returns NULL on allocation failure.
 */
char *str_to_cstr(struct str s);

#endif
