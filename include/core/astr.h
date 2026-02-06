#ifndef ASTR_H
#define ASTR_H

#include <stdarg.h>

#include "arena.h"
#include "str.h" /* Your existing str view type */

/*
 * astr - Arena-allocated string functions
 *
 * Unlike str (non-owning view), these functions copy data into the arena.
 * All returned strings are NUL-terminated for C interop.
 * Immutable after creation - create new str for modifications.
 * Functions abort on arena allocation failure.
 */

/* === Construction === */

/* Copy NUL-terminated C string into arena. Returns STR_EMPTY if s is NULL. */
struct str astr_from_cstr(struct arena *a, const char *s);

/* Copy str view into arena. The result is NUL-terminated. */
struct str astr_from_str(struct arena *a, struct str s);

/* === Formatting === */

/*
 * Create formatted string in arena (like sprintf).
 */
struct str astr_fmt(struct arena *a, const char *fmt, ...);

/* Like astr_fmt but takes va_list for wrapping in variadic functions. */
struct str astr_vfmt(struct arena *a, const char *fmt, va_list ap);

/* === Concatenation === */

/* Concatenate two strings into arena. */
struct str astr_cat(struct arena *a, struct str s1, struct str s2);

/* Concatenate three strings into arena. */
struct str astr_cat3(struct arena *a, struct str s1, struct str s2, struct str s3);

/* === Substring === */

/*
 * Extract substring copy starting at start with given length.
 * Clamps to valid range. Returns STR_EMPTY if start >= s.len.
 */
struct str astr_substr(struct arena *a, struct str s, int start, int len);

/* === Joining === */

/*
 * Join array of strings with separator between each part.
 * Returns STR_EMPTY if count is 0.
 */
struct str astr_join(struct arena *a, struct str sep, struct str *parts, int count);

/* === Path Operations === */

/*
 * Join directory and filename with path separator.
 * Handles trailing slash in dir correctly.
 */
struct str astr_path_join(struct arena *a, struct str dir, struct str file);

/*
 * Extract directory part of path (everything before last '/').
 * Returns "." if no slash found.
 */
struct str astr_path_dirname(struct arena *a, struct str path);

/*
 * Extract filename part of path (everything after last '/').
 * Returns entire path if no slash found.
 */
struct str astr_path_basename(struct arena *a, struct str path);

#endif
