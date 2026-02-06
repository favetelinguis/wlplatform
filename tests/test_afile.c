#include <assert.h>
#include <core/afile.h>
#include <core/arena.h>
#include <stdio.h>

static void
test_afile_read(void)
{
	struct arena a;
	arena_init(&a);

	/* Create test file */
	FILE *f = fopen("/tmp/test_afile.txt", "w");
	fprintf(f, "line1\nline2\nline3");
	fclose(f);

	struct afile_result r = afile_read(&a, "/tmp/test_afile.txt");
	assert(r.error == 0);
	assert(r.content.len == 17);

	arena_destroy(&a);
	remove("/tmp/test_afile.txt");
}

static void
test_afile_read_lines(void)
{
	struct arena a;
	arena_init(&a);

	FILE *f = fopen("/tmp/test_lines.txt", "w");
	fprintf(f, "alpha\nbeta\ngamma\n");
	fclose(f);

	struct afile_lines r = afile_read_lines(&a, "/tmp/test_lines.txt");
	assert(r.error == 0);
	assert(r.count == 4); /* 3 lines + empty after final \n */
	assert(str_eq(r.lines[0], STR_LIT("alpha")));
	assert(str_eq(r.lines[1], STR_LIT("beta")));
	assert(str_eq(r.lines[2], STR_LIT("gamma")));

	arena_destroy(&a);
	remove("/tmp/test_lines.txt");
}

static void
test_afile_not_found(void)
{
	struct arena a;
	arena_init(&a);

	struct afile_result r = afile_read(&a, "/nonexistent/path");
	assert(r.error == ENOENT);
	assert(r.content.data == NULL);

	arena_destroy(&a);
}

int
main(void)
{
	test_afile_read();
	test_afile_read_lines();
	test_afile_not_found();

	printf("All afile tests passed!\n");
	return 0;
}
