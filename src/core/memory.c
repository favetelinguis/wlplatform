#include <core/error.h>
#include <core/memory.h>
#include <stdlib.h>
#include <sys/mman.h>

void *
xmalloc(size_t size)
{
	void *p = malloc(size);
	if (!p)
		die_errno("malloc(%zu)", size);
	return p;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (!p)
		die_errno("calloc(%zu, %zu)", nmemb, size);
	return p;
}

void *
xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (!p && size != 0)
		die_errno("realloc(%zu)", size);
	return p;
}

void
xfree(void *ptr)
{
	free(ptr);
}

void *
xmmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *p = mmap(addr, length, prot, flags, fd, offset);
	if (p == MAP_FAILED)
		die_errno("mmap(%zu)", length);
	return p;
}

void
xmunmap(void *addr, size_t length)
{
	if (munmap(addr, length) != 0)
		die_errno("munmap(%zu)", length);
}
