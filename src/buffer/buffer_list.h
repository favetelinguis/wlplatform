/*
 * Manages multiple buffers. For now, we only use one,
 * but the abstraction allows easy extension when we add support for opening
 * multiple buffers.
 */

#define BUFFER_LIST_MAX 32 /* Max open buffers */

struct buffer_list {
	struct buffer buffers[BUFFER_LIST_MAX]; /* Number of open buffers */
	int count;				/* Number of open buffers */
	int active;				/* Index of curren buffer */
};
