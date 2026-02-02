#ifndef MYLIB_COMPAT_H
#define MYLIB_COMPAT_H

/*
 * C99 compatibility macros for C23 features
 *
 * This header provides C99-compatible replacements for C23 features:
 * - [[nodiscard]] -> NODISCARD (warn_unused_result)
 * - [[unsequenced]], [[reproducible]] -> removed (optimizer handles)
 * - nullptr -> NULL (use standard NULL)
 * - _Thread_local -> THREAD_LOCAL (__thread)
 * - alignas(N) -> ALIGNAS(N)
 * - alignof(T) -> ALIGNOF(T)
 * - _Static_assert -> STATIC_ASSERT
 */

#include <stddef.h>

/* max_align_t is C11; provide C99 fallback using long double (typically max aligned) */
#if __STDC_VERSION__ >= 201112L
/* C11 or later has max_align_t in stddef.h */
#else
/* C99 fallback: use a union of largest fundamental types */
typedef union {
	long long ll;
	long double ld;
	void *ptr;
} max_align_t;
#endif

/* Attribute macros - GCC/Clang extensions work with -std=c99 */
#if defined(__GNUC__) || defined(__clang__)
#define NODISCARD __attribute__((warn_unused_result))
#define ALIGNAS(n) __attribute__((aligned(n)))
#define ALIGNOF(t) __alignof__(t)
#define THREAD_LOCAL __thread
#else
#define NODISCARD
#define ALIGNAS(n)
#define ALIGNOF(t) sizeof(t)
#define THREAD_LOCAL
#endif

/* Static assert - C99 compatible via negative array trick */
#define STATIC_ASSERT(cond, msg) \
	typedef char static_assert_##msg[(cond) ? 1 : -1]

#endif /* MYLIB_COMPAT_H */
