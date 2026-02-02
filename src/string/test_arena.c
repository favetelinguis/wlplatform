#include "arena.h"
#include "compat.h"
#include "test_framework.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* === Basic allocation tests === */

TEST(arena_create_and_free) {
	arena *a = arena_create();
	ASSERT_NOT_NULL(a, "arena should be created");

	arena_free(a);
	/* No crash = success */
}

TEST(arena_create_with_custom_size) {
	arena *a = arena_create_with_size(1024);
	ASSERT_NOT_NULL(a, "arena with custom size should be created");

	arena_stats stats = arena_get_stats(a);
	ASSERT_TRUE(stats.total_allocated >= 1024, "should have allocated at least requested size");

	arena_free(a);
}

TEST(arena_alloc_basic) {
	arena *a = arena_create();

	int *ptr = arena_alloc(a, sizeof(int));
	ASSERT_NOT_NULL(ptr, "allocation should succeed");

	*ptr = 42;
	ASSERT_EQ(*ptr, 42, "allocated memory should be writable");

	arena_free(a);
}

TEST(arena_alloc_multiple) {
	arena *a = arena_create();

	int *a1 = arena_alloc(a, sizeof(int));
	int *a2 = arena_alloc(a, sizeof(int));
	int *a3 = arena_alloc(a, sizeof(int));

	ASSERT_NOT_NULL(a1, "first allocation should succeed");
	ASSERT_NOT_NULL(a2, "second allocation should succeed");
	ASSERT_NOT_NULL(a3, "third allocation should succeed");

	*a1 = 10;
	*a2 = 20;
	*a3 = 30;

	ASSERT_EQ(*a1, 10, "first value preserved");
	ASSERT_EQ(*a2, 20, "second value preserved");
	ASSERT_EQ(*a3, 30, "third value preserved");

	arena_free(a);
}

/* === Alignment tests === */

TEST(arena_alloc_aligned) {
	arena *a = arena_create();

	/* Test various alignments */
	void *p1 = arena_alloc_aligned(a, 1, 1);
	void *p2 = arena_alloc_aligned(a, 1, 2);
	void *p4 = arena_alloc_aligned(a, 1, 4);
	void *p8 = arena_alloc_aligned(a, 1, 8);
	void *p16 = arena_alloc_aligned(a, 1, 16);

	ASSERT_EQ((uintptr_t)p1 % 1, 0, "1-byte alignment");
	ASSERT_EQ((uintptr_t)p2 % 2, 0, "2-byte alignment");
	ASSERT_EQ((uintptr_t)p4 % 4, 0, "4-byte alignment");
	ASSERT_EQ((uintptr_t)p8 % 8, 0, "8-byte alignment");
	ASSERT_EQ((uintptr_t)p16 % 16, 0, "16-byte alignment");

	arena_free(a);
}

TEST(arena_alloc_struct_alignment) {
	arena *a = arena_create();

	typedef struct {
		char c;
		double d;
		int i;
	} TestStruct;

	TestStruct *s = arena_alloc_aligned(a, sizeof(TestStruct), ALIGNOF(TestStruct));
	ASSERT_NOT_NULL(s, "struct allocation should succeed");
	ASSERT_EQ((uintptr_t)s % ALIGNOF(TestStruct), 0, "struct should be properly aligned");

	s->c = 'A';
	s->d = 3.14;
	s->i = 42;

	ASSERT_EQ(s->c, 'A', "char field works");
	ASSERT_TRUE(s->d > 3.13 && s->d < 3.15, "double field works");
	ASSERT_EQ(s->i, 42, "int field works");

	arena_free(a);
}

/* === Zero-initialization tests === */

TEST(arena_alloc_zeroed) {
	arena *a = arena_create();

	size_t size = 100;
	char *buf = arena_alloc_zeroed(a, size);
	ASSERT_NOT_NULL(buf, "zeroed allocation should succeed");

	for (size_t i = 0; i < size; i++) {
		ASSERT_EQ(buf[i], 0, "all bytes should be zero");
	}

	arena_free(a);
}

/* === Type-safe macros tests === */

TEST(arena_new_single) {
	arena *a = arena_create();

	int *value = arena_new(a, int);
	ASSERT_NOT_NULL(value, "arena_new should allocate");
	ASSERT_EQ((uintptr_t)value % ALIGNOF(int), 0, "should be properly aligned");

	*value = 123;
	ASSERT_EQ(*value, 123, "value should be writable");

	arena_free(a);
}

TEST(arena_new_zeroed) {
	arena *a = arena_create();

	typedef struct { int x; int y; int z; } Point3D;
	Point3D *p = arena_new_zeroed(a, Point3D);

	ASSERT_NOT_NULL(p, "arena_new_zeroed should allocate");
	ASSERT_EQ(p->x, 0, "x should be zero");
	ASSERT_EQ(p->y, 0, "y should be zero");
	ASSERT_EQ(p->z, 0, "z should be zero");

	arena_free(a);
}

TEST(arena_new_array) {
	arena *a = arena_create();

	int *arr = arena_new_array(a, int, 10);
	ASSERT_NOT_NULL(arr, "array allocation should succeed");

	for (int i = 0; i < 10; i++) {
		arr[i] = i * 2;
	}

	for (int i = 0; i < 10; i++) {
		ASSERT_EQ(arr[i], i * 2, "array element should match");
	}

	arena_free(a);
}

TEST(arena_new_array_zeroed) {
	arena *a = arena_create();

	double *arr = arena_new_array_zeroed(a, double, 20);
	ASSERT_NOT_NULL(arr, "zeroed array allocation should succeed");

	for (int i = 0; i < 20; i++) {
		ASSERT_EQ(arr[i], 0.0, "array element should be zero");
	}

	arena_free(a);
}

/* === Reset tests === */

TEST(arena_reset_basic) {
	arena *a = arena_create();

	/* Allocate some memory */
	int *p1 = arena_alloc(a, sizeof(int));
	*p1 = 42;

	arena_stats before = arena_get_stats(a);
	ASSERT_TRUE(before.total_used > 0, "should have used memory");

	/* Reset */
	arena_reset(a);

	arena_stats after = arena_get_stats(a);
	ASSERT_EQ(after.total_used, 0, "used memory should be zero after reset");
	ASSERT_EQ(after.total_allocated, before.total_allocated, "allocated blocks should remain");

	/* Can allocate again */
	int *p2 = arena_alloc(a, sizeof(int));
	ASSERT_NOT_NULL(p2, "can allocate after reset");

	arena_free(a);
}

TEST(arena_reset_reuses_memory) {
	arena *a = arena_create_with_size(1024);

	/* Allocate something */
	void *p1 = arena_alloc(a, 100);
	arena_stats s1 = arena_get_stats(a);

	/* Reset and allocate again */
	arena_reset(a);
	void *p2 = arena_alloc(a, 100);
	arena_stats s2 = arena_get_stats(a);

	/* Should reuse the same block */
	ASSERT_EQ(s1.num_blocks, s2.num_blocks, "should not allocate new blocks");
	ASSERT_EQ(p1, p2, "should reuse same memory address");

	arena_free(a);
}

/* === Save/Restore tests === */

TEST(arena_save_restore_basic) {
	arena *a = arena_create();

	/* Allocate some data */
	int *p1 = arena_alloc(a, sizeof(int));
	*p1 = 10;

	/* Save state */
	arena_savepoint sp = arena_save(a);

	/* Allocate more data */
	int *p2 = arena_alloc(a, sizeof(int));
	*p2 = 20;

	arena_stats before_restore = arena_get_stats(a);

	/* Restore to savepoint */
	arena_restore(a, sp);

	arena_stats after_restore = arena_get_stats(a);

	/* Memory usage should be back to savepoint */
	ASSERT_TRUE(after_restore.total_used < before_restore.total_used,
	            "should have freed memory allocated after savepoint");

	/* Original data still valid */
	ASSERT_EQ(*p1, 10, "data before savepoint still accessible");

	arena_free(a);
}

TEST(arena_save_restore_multiple) {
	arena *a = arena_create();

	int *p1 = arena_alloc(a, sizeof(int));
	*p1 = 1;

	arena_savepoint sp1 = arena_save(a);

	int *p2 = arena_alloc(a, sizeof(int));
	*p2 = 2;

	arena_savepoint sp2 = arena_save(a);

	int *p3 = arena_alloc(a, sizeof(int));
	*p3 = 3;

	/* Restore to sp2 */
	arena_restore(a, sp2);
	ASSERT_EQ(*p1, 1, "p1 still valid");
	ASSERT_EQ(*p2, 2, "p2 still valid");

	/* Restore to sp1 */
	arena_restore(a, sp1);
	ASSERT_EQ(*p1, 1, "p1 still valid after second restore");

	arena_free(a);
}

/* === Statistics tests === */

TEST(arena_stats_basic) {
	arena *a = arena_create_with_size(1024);

	arena_stats s0 = arena_get_stats(a);
	ASSERT_EQ(s0.num_blocks, 1, "should start with one block");
	ASSERT_EQ(s0.total_used, 0, "should start with zero used");

	/* Allocate something */
	void *p1 = arena_alloc(a, 100);
	ASSERT_NOT_NULL(p1, "allocation should succeed");

	arena_stats s1 = arena_get_stats(a);
	ASSERT_EQ(s1.total_used, 100, "should track used memory");
	ASSERT_TRUE(s1.total_allocated >= 1024, "should track allocated memory");

	arena_free(a);
}

TEST(arena_stats_multiple_blocks) {
	arena *a = arena_create_with_size(4096);  /* 4KB blocks */

	/* Allocate enough to trigger multiple blocks (need > 4KB) */
	for (int i = 0; i < 100; i++) {
		void *p = arena_alloc(a, 64);
		(void)p;  /* Unused, just testing allocation */
	}

	arena_stats stats = arena_get_stats(a);
	ASSERT_TRUE(stats.num_blocks > 1, "should have multiple blocks");
	ASSERT_EQ(stats.total_used, 6400, "should track all used memory");

	arena_free(a);
}

/* === String duplication tests === */

TEST(arena_strdup_basic) {
	arena *a = arena_create();

	const char *original = "Hello, Arena!";
	char *copy = arena_strdup(a, original);

	ASSERT_NOT_NULL(copy, "strdup should succeed");
	ASSERT_TRUE(strcmp(copy, original) == 0, "strings should match");
	ASSERT_TRUE(copy != original, "should be different pointers");

	arena_free(a);
}

TEST(arena_strdup_multiple) {
	arena *a = arena_create();

	char *s1 = arena_strdup(a, "first");
	char *s2 = arena_strdup(a, "second");
	char *s3 = arena_strdup(a, "third");

	ASSERT_TRUE(strcmp(s1, "first") == 0, "first string correct");
	ASSERT_TRUE(strcmp(s2, "second") == 0, "second string correct");
	ASSERT_TRUE(strcmp(s3, "third") == 0, "third string correct");

	arena_free(a);
}

TEST(arena_memdup_basic) {
	arena *a = arena_create();

	int original[] = {1, 2, 3, 4, 5};
	int *copy = arena_memdup(a, original, sizeof(original));

	ASSERT_NOT_NULL(copy, "memdup should succeed");

	for (int i = 0; i < 5; i++) {
		ASSERT_EQ(copy[i], original[i], "array elements should match");
	}

	/* Modify copy - should not affect original */
	copy[0] = 999;
	ASSERT_EQ(original[0], 1, "original should be unchanged");
	ASSERT_EQ(copy[0], 999, "copy should be modified");

	arena_free(a);
}

/* === Large allocation tests === */

TEST(arena_large_allocation) {
	arena *a = arena_create_with_size(1024);  /* Small default size */

	/* Allocate something larger than default block */
	size_t large_size = 10 * 1024;
	void *ptr = arena_alloc(a, large_size);

	ASSERT_NOT_NULL(ptr, "large allocation should succeed");

	arena_stats stats = arena_get_stats(a);
	ASSERT_TRUE(stats.num_blocks >= 2, "should have created new block for large allocation");

	arena_free(a);
}

TEST(arena_many_small_allocations) {
	arena *a = arena_create();

	/* Allocate many small items */
	int **ptrs = malloc(sizeof(int *) * 1000);
	for (int i = 0; i < 1000; i++) {
		ptrs[i] = arena_alloc(a, sizeof(int));
		*ptrs[i] = i;
	}

	/* Verify all allocations */
	for (int i = 0; i < 1000; i++) {
		ASSERT_EQ(*ptrs[i], i, "all allocations should be preserved");
	}

	free(ptrs);
	arena_free(a);
}

/* === Temp arena tests === */

TEST(arena_temp_basic) {
	arena *temp = arena_temp_begin();
	ASSERT_NOT_NULL(temp, "temp arena should be available");

	int *data = arena_alloc(temp, sizeof(int) * 100);
	ASSERT_NOT_NULL(data, "can allocate from temp arena");

	arena_temp_end(temp);

	/* Can use temp arena again */
	arena *temp2 = arena_temp_begin();
	ASSERT_EQ(temp, temp2, "should reuse same temp arena");

	arena_temp_end(temp2);
}

TEST(arena_temp_with_savepoint) {
	arena *temp = arena_temp_get();

	/* Save and restore pattern */
	arena_savepoint sp = arena_save(temp);

	char *str = arena_strdup(temp, "temporary string");
	ASSERT_NOT_NULL(str, "can allocate strings in temp arena");

	arena_restore(temp, sp);

	/* Memory freed, can reuse */
	arena_stats stats = arena_get_stats(temp);
	ASSERT_EQ(stats.total_used, 0, "temp arena should be clean after restore");
}

/* === Edge cases === */

TEST(arena_alloc_zero_size) {
	arena *a = arena_create();

	void *ptr = arena_alloc(a, 0);
	ASSERT_NULL(ptr, "zero-size allocation should return NULL");

	arena_free(a);
}

TEST(arena_operations_on_null) {
	void *p = arena_alloc(NULL, 100);  /* Should not crash */
	ASSERT_NULL(p, "null arena alloc should return null");
	arena_reset(NULL);       /* Should not crash */
	arena_free(NULL);        /* Should not crash */

	arena_stats stats = arena_get_stats(NULL);
	ASSERT_EQ(stats.total_allocated, 0, "null arena stats should be zero");
}

int
main(void)
{
	printf("Running arena tests...\n\n");

	printf("Basic allocation:\n");
	RUN_TEST(arena_create_and_free);
	RUN_TEST(arena_create_with_custom_size);
	RUN_TEST(arena_alloc_basic);
	RUN_TEST(arena_alloc_multiple);

	printf("\nAlignment:\n");
	RUN_TEST(arena_alloc_aligned);
	RUN_TEST(arena_alloc_struct_alignment);

	printf("\nZero-initialization:\n");
	RUN_TEST(arena_alloc_zeroed);

	printf("\nType-safe macros:\n");
	RUN_TEST(arena_new_single);
	RUN_TEST(arena_new_zeroed);
	RUN_TEST(arena_new_array);
	RUN_TEST(arena_new_array_zeroed);

	printf("\nReset:\n");
	RUN_TEST(arena_reset_basic);
	RUN_TEST(arena_reset_reuses_memory);

	printf("\nSave/Restore:\n");
	RUN_TEST(arena_save_restore_basic);
	RUN_TEST(arena_save_restore_multiple);

	printf("\nStatistics:\n");
	RUN_TEST(arena_stats_basic);
	RUN_TEST(arena_stats_multiple_blocks);

	printf("\nString duplication:\n");
	RUN_TEST(arena_strdup_basic);
	RUN_TEST(arena_strdup_multiple);
	RUN_TEST(arena_memdup_basic);

	printf("\nLarge allocations:\n");
	RUN_TEST(arena_large_allocation);
	RUN_TEST(arena_many_small_allocations);

	printf("\nTemp arena:\n");
	RUN_TEST(arena_temp_basic);
	RUN_TEST(arena_temp_with_savepoint);

	printf("\nEdge cases:\n");
	RUN_TEST(arena_alloc_zero_size);
	RUN_TEST(arena_operations_on_null);

	printf("\n========================================\n");
	printf("All arena tests passed!\n");
	printf("========================================\n");

	return 0;
}
