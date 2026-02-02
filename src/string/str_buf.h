#ifndef MYLIB_STR_BUF_H
#define MYLIB_STR_BUF_H

#include "str.h"
#include "compat.h"
#include <stddef.h>
#include <stdbool.h>

/* Modern C99 mutable string buffer
 * OWNING: str_buf allocates and owns its data
 * MUST call str_buf_free() to avoid memory leaks
 */

/* Opaque type - internal structure hidden in implementation */
typedef struct str_buf str_buf;

/* === Lifecycle === */

/* Create new string buffer with default capacity */
NODISCARD str_buf *str_buf_new(void);

/* Create new string buffer with specified capacity */
NODISCARD str_buf *str_buf_with_capacity(size_t capacity);

/* Free string buffer and all associated memory */
void str_buf_free(str_buf *sb);

/* === Modification === */
/* Mutating operations - void return is intentional */

/* Append a single character */
void str_buf_push(str_buf *sb, char c);

/* Append a string view */
void str_buf_append(str_buf *sb, str s);

/* Append a C string */
void str_buf_append_cstr(str_buf *sb, const char *cstr);

/* Clear the buffer (keeps capacity, resets length) */
void str_buf_clear(str_buf *sb);

/* Reserve additional capacity */
void str_buf_reserve(str_buf *sb, size_t additional);

/* === Inspection === */
/* Non-mutating query functions */

/* Get current length */
NODISCARD size_t str_buf_len(const str_buf *sb);

/* Get current capacity */
NODISCARD size_t str_buf_capacity(const str_buf *sb);

/* === Conversion/Views === */
/* BORROWED REFERENCE: Valid only while str_buf is alive */

/* Get immutable view of buffer contents */
NODISCARD str str_buf_view(const str_buf *sb);

/* Get null-terminated C string view (valid while buffer lives) */
NODISCARD const char *str_buf_cstr(const str_buf *sb);

#endif /* MYLIB_STR_BUF_H */
