#include "arena.h"
#include "compat.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Default block size: 64KB (good balance for most use cases) */
#define DEFAULT_BLOCK_SIZE (64 * 1024)

/* Minimum block size: 4KB */
#define MIN_BLOCK_SIZE (4 * 1024)

/* Arena block - uses flexible array member */
struct arena_block {
	struct arena_block *next;
	size_t size;              /* Total size of data array */
	size_t used;              /* Bytes currently used */
	ALIGNAS(sizeof(max_align_t)) char data[];  /* Flexible array member */
};

/* Arena structure */
struct arena {
	struct arena_block *current;      /* Current block for allocations */
	struct arena_block *blocks;       /* Head of block chain (most recent) */
	size_t default_block_size;        /* Size for new blocks */
	size_t total_allocated;           /* Total bytes allocated from system */
	size_t total_wasted;              /* Bytes wasted due to alignment */
};

/* Thread-local scratch arena for temporary allocations */
static THREAD_LOCAL arena *scratch_arena = NULL;

/* === Internal Helpers === */

static struct arena_block *
block_create(size_t size)
{
	if (size < MIN_BLOCK_SIZE) {
		size = MIN_BLOCK_SIZE;
	}

	struct arena_block *block = malloc(sizeof(struct arena_block) + size);
	if (block == NULL) {
		return NULL;
	}

	block->next = NULL;
	block->size = size;
	block->used = 0;

	return block;
}

static inline size_t
align_forward(size_t addr, size_t align)
{
	return (addr + align - 1) & ~(align - 1);
}

/* === Core Operations === */

arena *
arena_create(void)
{
	return arena_create_with_size(DEFAULT_BLOCK_SIZE);
}

arena *
arena_create_with_size(size_t initial_size)
{
	arena *a = malloc(sizeof(arena));
	if (a == NULL) {
		return NULL;
	}

	struct arena_block *block = block_create(initial_size);
	if (block == NULL) {
		free(a);
		return NULL;
	}

	a->current = block;
	a->blocks = block;
	a->default_block_size = initial_size >= MIN_BLOCK_SIZE ? initial_size : DEFAULT_BLOCK_SIZE;
	a->total_allocated = sizeof(struct arena_block) + block->size;
	a->total_wasted = 0;

	return a;
}

void *
arena_alloc_aligned(arena *a, size_t size, size_t align)
{
	if (a == NULL || size == 0) {
		return NULL;
	}

	/* Ensure alignment is power of 2 */
	if (align == 0 || (align & (align - 1)) != 0) {
		align = ALIGNOF(max_align_t);
	}

	/* Calculate aligned address within current block */
	uintptr_t curr = (uintptr_t)(a->current->data + a->current->used);
	uintptr_t aligned = align_forward(curr, align);
	size_t padding = aligned - curr;

	/* Check if allocation fits in current block */
	if (a->current->used + padding + size <= a->current->size) {
		void *ptr = (void *)aligned;
		a->current->used += padding + size;
		a->total_wasted += padding;
		return ptr;
	}

	/* Need a new block */
	/* For large allocations, create a block that fits (1.5x size) */
	/* For normal allocations, use default block size */
	size_t block_size = size > a->default_block_size / 2
	                    ? size + (size / 2)
	                    : a->default_block_size;

	struct arena_block *block = block_create(block_size);
	if (block == NULL) {
		return NULL;
	}

	/* Add block to chain */
	block->next = a->blocks;
	a->blocks = block;
	a->current = block;
	a->total_allocated += sizeof(struct arena_block) + block->size;

	/* Track wasted space in previous block */
	size_t prev_block_waste = a->current->next->size - a->current->next->used;
	a->total_wasted += prev_block_waste;

	/* Allocate from new block (already aligned) */
	block->used = size;
	return block->data;
}

void *
arena_alloc(arena *a, size_t size)
{
	return arena_alloc_aligned(a, size, ALIGNOF(max_align_t));
}

void *
arena_alloc_zeroed_aligned(arena *a, size_t size, size_t align)
{
	void *ptr = arena_alloc_aligned(a, size, align);
	if (ptr != NULL) {
		memset(ptr, 0, size);
	}
	return ptr;
}

void *
arena_alloc_zeroed(arena *a, size_t size)
{
	return arena_alloc_zeroed_aligned(a, size, ALIGNOF(max_align_t));
}

void
arena_reset(arena *a)
{
	if (a == NULL) {
		return;
	}

	/* Reset all blocks */
	for (struct arena_block *block = a->blocks; block != NULL; block = block->next) {
		block->used = 0;
	}

	/* Reset current to most recent block */
	a->current = a->blocks;
	a->total_wasted = 0;
}

void
arena_free(arena *a)
{
	if (a == NULL) {
		return;
	}

	/* Free all blocks */
	struct arena_block *block = a->blocks;
	while (block != NULL) {
		struct arena_block *next = block->next;
		free(block);
		block = next;
	}

	free(a);
}

/* === Save/Restore Points === */

arena_savepoint
arena_save(arena *a)
{
	if (a == NULL || a->current == NULL) {
		return (arena_savepoint){.block = NULL, .used = 0};
	}

	return (arena_savepoint){
		.block = a->current,
		.used = a->current->used
	};
}

void
arena_restore(arena *a, arena_savepoint sp)
{
	if (a == NULL || sp.block == NULL) {
		return;
	}

	/* Restore the saved block as current */
	a->current = (struct arena_block *)sp.block;
	a->current->used = sp.used;

	/* Note: We don't free blocks allocated after the savepoint */
	/* They remain in the chain for reuse on next reset */
}

/* === Statistics === */

arena_stats
arena_get_stats(arena *a)
{
	if (a == NULL) {
		return (arena_stats){0};
	}

	size_t total_used = 0;
	size_t num_blocks = 0;

	for (struct arena_block *block = a->blocks; block != NULL; block = block->next) {
		total_used += block->used;
		num_blocks++;
	}

	return (arena_stats){
		.total_allocated = a->total_allocated,
		.total_used = total_used,
		.total_wasted = a->total_wasted,
		.num_blocks = num_blocks,
		.current_block_size = a->current ? a->current->size : 0,
		.current_block_used = a->current ? a->current->used : 0
	};
}

/* === String Duplication === */

char *
arena_strdup(arena *a, const char *s)
{
	if (a == NULL || s == NULL) {
		return NULL;
	}

	size_t len = strlen(s);
	char *copy = arena_alloc(a, len + 1);
	if (copy == NULL) {
		return NULL;
	}

	memcpy(copy, s, len + 1);
	return copy;
}

void *
arena_memdup(arena *a, const void *src, size_t size)
{
	if (a == NULL || src == NULL || size == 0) {
		return NULL;
	}

	void *copy = arena_alloc(a, size);
	if (copy == NULL) {
		return NULL;
	}

	memcpy(copy, src, size);
	return copy;
}

/* === Temp Arena Pattern === */

arena *
arena_temp_get(void)
{
	if (scratch_arena == NULL) {
		scratch_arena = arena_create();
	}
	return scratch_arena;
}

arena *
arena_temp_begin(void)
{
	arena *temp = arena_temp_get();
	arena_reset(temp);
	return temp;
}

void
arena_temp_end(arena *a)
{
	/* For temp arenas, we just reset them */
	/* They're reused in the next temp_begin */
	arena_reset(a);
}
