#include <core/afile.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int
afile_exists(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0;
}

long
afile_size(const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return -1;
	return (long)st.st_size;
}

struct afile_result
afile_read(struct arena *a, const char *path)
{
	struct afile_result r = {0};
	FILE *f;
	long size;
	char *buf;
	size_t nread;

	f = fopen(path, "rb");
	if (!f) {
		r.error = errno;
		return r;
	}

	/* Get file size */
	if (fseek(f, 0, SEEK_END) != 0) {
		r.error = errno;
		fclose(f);
		return r;
	}
	size = ftell(f);
	if (size < 0) {
		r.error = errno;
		fclose(f);
		return r;
	}
	rewind(f);

	/* Allocate and read */
	buf = arena_alloc(a, (size_t)size + 1, 1);
	nread = fread(buf, 1, (size_t)size, f);
	fclose(f);

	if (nread != (size_t)size) {
		r.error = EIO;
		return r;
	}

	buf[size] = '\0';
	r.content = (struct str){buf, (int)size};
	return r;
}

struct afile_result
afile_read_str(struct arena *a, struct str path)
{
	char *cpath;
	struct afile_result r;

	/* Temporary C string for path */
	cpath = arena_alloc(a, (size_t)path.len + 1, 1);
	memcpy(cpath, path.data, (size_t)path.len);
	cpath[path.len] = '\0';

	r = afile_read(a, cpath);
	return r;
}

struct afile_lines
afile_read_lines(struct arena *a, const char *path)
{
	struct afile_lines r = {0};
	struct afile_result file;
	int count, i;
	const char *p, *end, *line_start;
	struct str *lines;

	file = afile_read(a, path);
	if (file.error) {
		r.error = file.error;
		return r;
	}

	if (file.content.len == 0) {
		r.lines = NULL;
		r.count = 0;
		return r;
	}

	/* Count lines */
	count = 1;
	p = file.content.data;
	end = p + file.content.len;
	while (p < end) {
		if (*p == '\n')
			count++;
		p++;
	}

	/* Allocate line array */
	lines = arena_array(a, struct str, count);

	/* Split into lines */
	p = file.content.data;
	line_start = p;
	i = 0;
	while (p <= end) {
		if (p == end || *p == '\n') {
			lines[i].data = line_start;
			lines[i].len = (int)(p - line_start);
			/* Strip \r if present */
			if (lines[i].len > 0 &&
			    line_start[lines[i].len - 1] == '\r')
				lines[i].len--;
			i++;
			line_start = p + 1;
		}
		p++;
	}

	r.lines = lines;
	r.count = i;
	return r;
}
