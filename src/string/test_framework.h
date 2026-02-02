#ifndef MYLIB_TEST_FRAMEWORK_H
#define MYLIB_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/*
 * Simple test framework for C99
 * Provides basic assertion macros and test definition helpers
 */

/* Basic assertion - fails test if condition is false */
#define ASSERT(cond, msg) \
	do { \
		if (!(cond)) { \
			fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg); \
			exit(1); \
		} \
	} while(0)

/* Assert two values are equal */
#define ASSERT_EQ(a, b, msg) ASSERT((a) == (b), msg)

/* Assert two values are not equal */
#define ASSERT_NE(a, b, msg) ASSERT((a) != (b), msg)

/* Assert pointer is null */
#define ASSERT_NULL(ptr, msg) ASSERT((ptr) == NULL, msg)

/* Assert pointer is not null */
#define ASSERT_NOT_NULL(ptr, msg) ASSERT((ptr) != NULL, msg)

/* Assert condition is true */
#define ASSERT_TRUE(cond, msg) ASSERT((cond), msg)

/* Assert condition is false */
#define ASSERT_FALSE(cond, msg) ASSERT(!(cond), msg)

/* Define a test function
 * Usage: TEST(my_test_name) { ... test body ... }
 */
#define TEST(name) \
	static void test_##name(void); \
	static void test_##name(void)

/* Run a test and print result */
#define RUN_TEST(name) \
	do { \
		printf("Running test_%s...\n", #name); \
		test_##name(); \
		printf("  PASS\n"); \
	} while(0)

#endif /* MYLIB_TEST_FRAMEWORK_H */
