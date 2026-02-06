#include <assert.h>
#include <core/arena.h>
#include <stdio.h>
#include <string.h>

static void
test_basic_alloc(void)
{
	struct arena a;
	arena_init(&a);

	int *x = arena_new(&a, int);
	*x = 42;
	assert(*x == 42);

	int *arr = arena_array(&a, int, 100);
	for (int i = 0; i < 100; i++)
		arr[i] = i;
	assert(arr[50] == 50);

	arena_destroy(&a);
}

static void
test_mark_pop(void)
{
	struct arena a;
	arena_init(&a);

	int *persistent = arena_new(&a, int);
	*persistent = 100;

	struct arena_mark m = arena_mark(&a);

	int *temp = arena_array(&a, int, 50);
	temp[0] = 999;

	arena_pop(&a, m);

	assert(*persistent == 100);

	/* New alloc reuses space */
	int *reused = arena_new(&a, int);
	(void)reused;

	arena_destroy(&a);
}

static void
test_reset(void)
{
	struct arena a;
	arena_init(&a);

	for (int round = 0; round < 10; round++) {
		for (int i = 0; i < 100; i++)
			arena_new(&a, int);
		arena_reset(&a);
	}

	arena_destroy(&a);
}

static void
test_zero_alloc(void)
{
	struct arena a;
	arena_init(&a);

	int *arr = arena_array0(&a, int, 10);
	for (int i = 0; i < 10; i++)
		assert(arr[i] == 0);

	arena_destroy(&a);
}

static void
test_large_alloc(void)
{
	struct arena a;
	arena_init(&a);

	/* Larger than default block size - triggers oversized block */
	char *big = arena_alloc(&a, 100000, 1);
	memset(big, 'x', 100000);
	assert(big[99999] == 'x');

	arena_destroy(&a);
}

int
main(void)
{
	test_basic_alloc();
	test_mark_pop();
	test_reset();
	test_zero_alloc();
	test_large_alloc();

	printf("All arena tests passed!\n");
	return 0;
}
