#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <sys/mman.h>

void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
void xfree(void *ptr);

void *
xmmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
void xmunmap(void *addr, size_t length);

#endif
