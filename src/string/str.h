#ifndef MYLIB_STR_H
#define MYLIB_STR_H

#include <stddef.h>
#include <stdbool.h>
#include "compat.h"

/* Modern C99 immutable string view
 * NON-OWNING: str is just a view into existing memory
 * Lifetime: valid as long as the underlying data is valid
 */

typedef struct {
	const char *data;  /* Non-owning pointer */
	size_t len;        /* Length in bytes (not including null terminator) */
} str;

/* Constant for empty string */
#define STR_EMPTY ((str){.data = "", .len = 0})

/* Convenience macro for string literals using compound literal + designated initializer */
#define STR_LIT(s) ((str){.data = (s), .len = sizeof(s) - 1})

/* === Construction === */
/* Pass str by value (it's just 2 words) */

/* Create str from C string (calculates length) */
NODISCARD str str_from_cstr(const char *cstr);

/* Create str from pointer and length */
NODISCARD str str_from_parts(const char *data, size_t len);

/* Create substring slice [start, end) */
NODISCARD str str_slice(str s, size_t start, size_t end);

/* === Inspection === */

/* Get length of string */
NODISCARD static inline size_t
str_len(str s)
{
	return s.len;
}

/* Check if string is empty */
NODISCARD static inline bool
str_empty(str s)
{
	return s.len == 0;
}

/* Get pointer to data */
NODISCARD static inline const char *
str_data(str s)
{
	return s.data;
}

/* === Character Access === */
/* Return "option" type instead of sentinel for type safety */

typedef struct {
	char value;
	bool valid;
} str_char_opt;

/* Safely get character at index (returns {0, false} if out of bounds) */
NODISCARD str_char_opt str_at(str s, size_t index);

/* === Comparison === */

/* Check if two strings are equal */
NODISCARD bool str_eq(str a, str b);

/* Compare two strings lexicographically (-1, 0, 1) */
NODISCARD int str_cmp(str a, str b);

/* Check if string starts with prefix */
NODISCARD bool str_starts_with(str s, str prefix);

/* Check if string ends with suffix */
NODISCARD bool str_ends_with(str s, str suffix);

/* === Search === */
/* Return struct instead of -1 sentinel for type safety */

typedef struct {
	size_t index;
	bool found;
} str_search_result;

/* Find first occurrence of needle in string */
NODISCARD str_search_result str_find(str s, str needle);

/* Find last occurrence of needle in string */
NODISCARD str_search_result str_rfind(str s, str needle);

/* === Conversion === */

/* Convert to null-terminated C string (allocates memory - caller must free) */
NODISCARD char *str_to_cstr(str s);

/* === Compile-time invariants === */
STATIC_ASSERT(sizeof(str) == 2 * sizeof(void *), str_should_be_two_pointers);

#endif /* MYLIB_STR_H */
