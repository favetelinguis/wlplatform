#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

#define ARENA_BLOCK_SIZE (64u * 1024u)
#define ARENA_MIN_ALIGN	 sizeof(void *)

struct arena_block {
	struct arena_block *next;
	size_t cap; /* bytes avaliable in data[] */
	size_t pos; /* bump cursor */
	uint8_t data[] __attribute__((aligned(16)));
};

struct arena {
	struct arena_block *head;
	struct arena_block *curr;
};

struct arena_mark {
	struct arena_block *block;
	size_t pos;
};

/* Lifecycle - these abort on failure, never return NULL */

/* Initialize arena with first block. Aborts on allocation failure. */
void arena_init(struct arena *a);

/* Free all blocks and zero the arena. Safe to call on uninitialized arena. */
void arena_destroy(struct arena *a);

/* Free extra blocks, keep first block, reset position to start. */
void arena_reset(struct arena *a);

/* Allocation - aborts on failure, never returns NULL */

/*
 * Allocate size bytes with given alignment from arena.
 * Allocates new block if current block is full.
 * Returns uninitialized memory. Aborts on failure.
 */
void *arena_alloc(struct arena *a, size_t size, size_t align);

/*
 * Like arena_alloc but zero-initializes the memory.
 */
void *arena_alloc0(struct arena *a, size_t size, size_t align);

/* Scratch/temporary allocation support */

/* Save current arena position for later restoration with arena_pop. */
struct arena_mark arena_mark(struct arena *a);

/*
 * Restore arena to saved position, freeing all allocations made after mark.
 * Frees any blocks allocated after the marked block.
 */
void arena_pop(struct arena *a, struct arena_mark m);

/* Convenience macros - use these instead of arena_alloc directly */

/* Allocate single instance of type T (uninitialized) */
#define arena_new(a, T)	 ((T *)arena_alloc((a), sizeof(T), __alignof__(T)))

/* Allocate single instance of type T (zero-initialized) */
#define arena_new0(a, T) ((T *)arena_alloc0((a), sizeof(T), __alignof__(T)))

/* Allocate array of n elements of type T (uninitialized) */
#define arena_array(a, T, n)                                                  \
	((T *)arena_alloc((a), sizeof(T) * (n), __alignof__(T)))

/* Allocate array of n elements of type T (zero-initialized) */
#define arena_array0(a, T, n)                                                 \
	((T *)arena_alloc0((a), sizeof(T) * (n), __alignof__(T)))

#endif
