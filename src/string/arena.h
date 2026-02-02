#ifndef MYLIB_ARENA_H
#define MYLIB_ARENA_H

#include <stddef.h>
#include <stdbool.h>
#include "compat.h"

/*
 * Modern arena allocator for C99
 *
 * Arena allocators (also called bump allocators or region allocators) are
 * a fast allocation strategy where memory is allocated sequentially from
 * large blocks. Individual allocations cannot be freed - instead, the entire
 * arena is freed at once or reset for reuse.
 *
 * Benefits:
 * - Very fast allocation (just pointer bump)
 * - No per-allocation overhead (no metadata per allocation)
 * - No fragmentation within blocks
 * - Natural lifetime management (all allocations freed together)
 * - Excellent cache locality (sequential allocations)
 *
 * Common use cases:
 * - Temporary/scratch memory during function execution
 * - Per-frame allocations in games
 * - Per-request allocations in servers
 * - Building data structures that are freed together
 * - String processing pipelines
 */

/* === Core Arena Types === */

typedef struct arena arena;

/* Save point for temporary allocations (stack-like usage) */
typedef struct {
	void *block;      /* Current block at save time */
	size_t used;      /* Bytes used in block at save time */
} arena_savepoint;

/* Arena statistics */
typedef struct {
	size_t total_allocated;   /* Total bytes allocated from system */
	size_t total_used;        /* Total bytes used by allocations */
	size_t total_wasted;      /* Bytes wasted due to alignment/padding */
	size_t num_blocks;        /* Number of blocks in chain */
	size_t current_block_size; /* Size of current block */
	size_t current_block_used; /* Bytes used in current block */
} arena_stats;

/* === Core Operations === */

/* Create a new arena with default block size (64KB) */
NODISCARD arena *arena_create(void);

/* Create arena with custom initial block size */
NODISCARD arena *arena_create_with_size(size_t initial_size);

/* Allocate memory from arena with default alignment (alignof(max_align_t)) */
NODISCARD void *arena_alloc(arena *a, size_t size);

/* Allocate memory with specific alignment */
NODISCARD void *arena_alloc_aligned(arena *a, size_t size, size_t align);

/* Allocate and zero-initialize */
NODISCARD void *arena_alloc_zeroed(arena *a, size_t size);

/* Reset arena (keeps blocks, resets pointers for reuse) */
void arena_reset(arena *a);

/* Free all arena memory */
void arena_free(arena *a);

/* === Type-Safe Allocation Helpers === */

/* Allocate single item of type T */
#define arena_new(arena, T) \
	((T *)arena_alloc_aligned(arena, sizeof(T), ALIGNOF(T)))

/* Allocate and zero-initialize single item of type T */
#define arena_new_zeroed(arena, T) \
	((T *)arena_alloc_zeroed_aligned(arena, sizeof(T), ALIGNOF(T)))

/* Allocate array of n items of type T */
#define arena_new_array(arena, T, n) \
	((T *)arena_alloc_aligned(arena, sizeof(T) * (n), ALIGNOF(T)))

/* Allocate and zero-initialize array of n items of type T */
#define arena_new_array_zeroed(arena, T, n) \
	((T *)arena_alloc_zeroed_aligned(arena, sizeof(T) * (n), ALIGNOF(T)))

/* === Save/Restore Points (Temporary Allocations) === */

/* Save current arena state for later restoration
 * Enables stack-like temporary allocation patterns */
arena_savepoint arena_save(arena *a);

/* Restore arena to saved state (frees all allocations since save) */
void arena_restore(arena *a, arena_savepoint sp);

/* === Statistics === */

/* Get arena statistics */
arena_stats arena_get_stats(arena *a);

/* === String Duplication === */

/* Duplicate C string into arena (allocates strlen(s) + 1 bytes) */
NODISCARD char *arena_strdup(arena *a, const char *s);

/* Duplicate memory region into arena */
NODISCARD void *arena_memdup(arena *a, const void *src, size_t size);

/* === Temp Arena Pattern === */

/*
 * Scratch/temporary arena for short-lived allocations
 * This is a common pattern for temporary work within a function
 *
 * Usage:
 *   arena *temp = arena_temp_begin();
 *   ... do temporary allocations ...
 *   arena_temp_end(temp);
 *
 * Or with save/restore:
 *   arena *temp = get_scratch_arena();
 *   arena_savepoint sp = arena_save(temp);
 *   ... do temporary allocations ...
 *   arena_restore(temp, sp);
 */

/* Thread-local scratch arena (lazily initialized) */
arena *arena_temp_get(void);

/* Begin temporary allocation session (returns fresh arena or savepoint) */
arena *arena_temp_begin(void);

/* End temporary session (resets arena) */
void arena_temp_end(arena *a);

/* === Internal Helpers === */

/* Allocate and zero-initialize with specific alignment */
void *arena_alloc_zeroed_aligned(arena *a, size_t size, size_t align);

#endif /* MYLIB_ARENA_H */
