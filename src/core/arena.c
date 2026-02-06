#include <core/arena.h>
#include <core/error.h>
#include <core/memory.h>
#include <string.h>

static size_t
align_up(size_t p, size_t a)
{
	return (p + (a - 1)) & ~(a - 1);
}

static struct arena_block *
block_new(size_t cap)
{
	struct arena_block *b;
	b = xmalloc(sizeof(*b) + cap);
	b->next = NULL;
	b->cap = cap;
	b->pos = 0;
	return b;
}

void
arena_init(struct arena *a)
{
	memset(a, 0, sizeof(*a));
	a->head = a->curr = block_new(ARENA_BLOCK_SIZE);
}

void
arena_destroy(struct arena *a)
{
	struct arena_block *b, *next;
	if (!a)
		return;

	for (b = a->head; b; b = next) {
		next = b->next;
		xfree(b);
	}
	memset(a, 0, sizeof(*a));
}

void
arena_reset(struct arena *a)
{
	struct arena_block *b, *next;
	if (!a || !a->head)
		return;

	/* Free all blocks except first */
	for (b = a->head->next; b; b = next) {
		next = b->next;
		xfree(b);
	}

	a->head->next = NULL;
	a->head->pos = 0;
	a->curr = a->head;
}

struct arena_mark
arena_mark(struct arena *a)
{
	return (struct arena_mark){.block = a->curr,
				   .pos = a->curr ? a->curr->pos : 0};
}

void
arena_pop(struct arena *a, struct arena_mark m)
{
	struct arena_block *b, *next;
	if (!a || !m.block)
		return;

	/* Free block allocated after mark */
	for (b = m.block->next; b; b = next) {
		next = b->next;
		xfree(b);
	}

	m.block->next = NULL;
	m.block->pos = m.pos;
	a->curr = m.block;
}

static void *
alloc_from_block(struct arena_block *b, size_t size, size_t align)
{
	size_t p;

	p = align_up(b->pos, align);

	/* Overflow-safe check */
	if (size > b->cap - p)
		return NULL;

	b->pos = p + size;
	return b->data + p;
}

void *
arena_alloc(struct arena *a, size_t size, size_t align)
{
	void *p;
	size_t min_cap, cap;
	struct arena_block *nb;

	if (!a || !a->curr)
		die("arena: not initialized");

	if (size == 0)
		size = 1;
	if (align < ARENA_MIN_ALIGN)
		align = ARENA_MIN_ALIGN;
	if ((align & (align - 1)) != 0)
		die("arena:alignment must be power of two");

	/* Try current block */
	p = alloc_from_block(a->curr, size, align);
	if (p)
		return p;

	/* Allocate new block */
	min_cap = size + (align - 1);
	cap = ARENA_BLOCK_SIZE;
	if (cap < min_cap)
		cap = min_cap;

	nb = block_new(cap);
	a->curr->next = nb;
	a->curr = nb;

	p = alloc_from_block(nb, size, align);
	if (!p)
		die("arena: allocation failed after new block");

	return p;
}

void *
arena_alloc0(struct arena *a, size_t size, size_t align)
{
	void *p = arena_alloc(a, size, align);
	memset(p, 0, size);
	return p;
}
