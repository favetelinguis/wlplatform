#ifndef UI_AVY_H
#define UI_AVY_H

#include <stdbool.h>
#include <stdint.h>

#include "../buffer/buffer.h"
#include "../string/str.h"
#include "ui_types.h"

/*
 * Maximum matches we can track.
 * With hint chars "asdfghjkl" (9 chars):
 *   - Single char hints: 9
 *   - Double char hints: 81
 *   - Total: 90 addressable targets
 * We allow 100 to have some headroom.
 */
#define AVY_MAX_MATCHES 100

/*
 * Hint characters in priority order.
 * Home row keys for minimal finger movement.
 * 9 characters = 9 single-char + 81 double-char = 90 total hints.
 */
#define AVY_HINT_CHARS "asdfghjkl"

/* Direction of search from cursor */
enum avy_direction {
	AVY_DIR_UP,
	AVY_DIR_DOWN,
};

/*
 * A single match location.
 *
 * Memory layout (12 bytes, no padding on most architectures):
 *   line: 4 bytes
 *   col:  4 bytes
 *   hint: 3 bytes + 1 padding
 *
 * For 100 matches: 1200 bytes, fits comfortably in L1 cache.
 */
struct avy_match {
	int line;     /* Buffer line number (0-indexed) */
	int col;      /* Column byte offset where hint appears */
	char hint[3]; /* 1-2 char hint + null terminator */
};

/*
 * Avy state machine.
 *
 * Design notes:
 * - All state is value types, no heap allocation
 * - matches[] is a simple array; we use linear scan for lookup
 *   (see Data Structure Analysis section for rationale)
 * - selected_match is an index into matches[], not a pointer,
 *   to avoid dangling pointer issues if matches were reallocated
 */
struct avy_state {
	bool active;		      /* Currently in avy mode */
	enum avy_direction direction; /* Search direction */
	char search_char;	      /* Character we searched for */
	struct avy_match matches[AVY_MAX_MATCHES];
	int match_count;
	int selected_match; /* Index into matches[], -1 if none */
	char hint_input[3]; /* Partial hint being typed */
	int hint_input_len;
};

/* Initialize avy state (call once at startup) */
void avy_init(struct avy_state *avy);

/* Start avy mode in given direction */
void avy_start(struct avy_state *avy, enum avy_direction dir);

/* Cancel avy mode, return to normal */
void avy_cancel(struct avy_state *avy);

/*
 * Set search character and find matches in visible lines.
 * Populates matches[] array and generates hint labels.
 */
void avy_set_char(struct avy_state *avy,
		  char c,
		  struct buffer *buf,
		  int cursor_line,
		  int first_visible,
		  int last_visible);

/*
 * Process a hint character input.
 * Returns true if selection is complete (unique match found).
 * Returns false if more input needed or no matches.
 */
bool avy_input_hint(struct avy_state *avy, char c);

/* Get the selected match (only valid after avy_input_hint returns true) */
struct avy_match *avy_get_selected(struct avy_state *avy);

/*
 * Draw hint overlays on visible buffer lines.
 *
 * Parameters:
 *   ctx              - Rendering context
 *   avy              - Avy state with matches to display
 *   line_y_positions - Array mapping visible line index to Y pixel position
 *   line_count       - Number of entries in line_y_positions
 *   first_visible_line - Buffer line number of first visible line
 *   cursor_line      - Buffer line number of cursor (rendered as input box)
 *   padding_x        - Left padding in pixels
 */
void avy_draw_hints(struct ui_ctx *ctx,
		    struct avy_state *avy,
		    int *line_y_positions,
		    int line_count,
		    int first_visible_line,
		    int cursor_line,
		    int padding_x);

#endif /* UI_AVY_H */
