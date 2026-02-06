/*
 * error.h - fatal error handling for suckless-style C
 *
 * All functions use printf-style format strings.
 * Output goes to stderr with "error:" or "warning:" prefix.
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

/*
 * Print formatted error message to stderr and exit(1).
 * Format: "error: <message>\n"
 */
void die(const char *fmt, ...) __attribute__((noreturn));

/*
 * Print formatted error with errno description and exit(1).
 * Format: "error: <message>: <strerror(errno)>\n"
 */
void die_errno(const char *fmt, ...) __attribute__((noreturn));

/*
 * Print formatted warning to stderr (does not exit).
 * Format: "warning: <message>\n"
 */
void warn(const char *fmt, ...);

/*
 * Print warning with errno description (does not exit).
 * Format: "warning: <message>: <strerror(errno)>\n"
 */
void warn_errno(const char *fmt, ...);

/*
 * Print debug message to stderr (compiles out with NDEBUG).
 * Format: "dbg: <message>\n"
 */
#ifdef NDEBUG
#define dbg(...) ((void)0)
#else
#define dbg(...) warn("dbg: " __VA_ARGS__)
#endif

#endif
