#include <assert.h>
#include <core/astr.h>
#include <stdio.h>
#include <string.h>

static void
test_astr_from_cstr(void)
{
	struct arena a;
	arena_init(&a);

	struct str s = astr_from_cstr(&a, "hello");
	assert(s.len == 5);
	assert(memcmp(s.data, "hello", 5) == 0);
	assert(s.data[5] == '\0');

	arena_destroy(&a);
}

static void
test_astr_fmt(void)
{
	struct arena a;
	arena_init(&a);

	struct str s = astr_fmt(&a, "count: %d, name: %s", 42, "test");
	assert(strcmp(s.data, "count: 42, name: test") == 0);

	arena_destroy(&a);
}

static void
test_astr_cat(void)
{
	struct arena a;
	arena_init(&a);

	struct str s = astr_cat(&a, STR_LIT("hello"), STR_LIT(" world"));
	assert(s.len == 11);
	assert(strcmp(s.data, "hello world") == 0);

	arena_destroy(&a);
}

static void
test_astr_join(void)
{
	struct arena a;
	arena_init(&a);

	struct str parts[] = {STR_LIT("a"), STR_LIT("b"), STR_LIT("c")};
	struct str s = astr_join(&a, STR_LIT(", "), parts, 3);
	assert(strcmp(s.data, "a, b, c") == 0);

	arena_destroy(&a);
}

int
main(void)
{
	test_astr_from_cstr();
	test_astr_fmt();
	test_astr_cat();
	test_astr_join();

	printf("All astr tests passed!\n");
	return 0;
}
