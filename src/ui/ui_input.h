#ifndef UI_INPUT_H
#define UI_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#define UI_INPUT_MAX_LEN 255

/*
 * Single-line text input state.
 *
 * This compnent is always focused-no focus flag needed.
 * Caller owns this struct; ui_input functions operate on it.
 */
struct ui_input {
	char buf[UI_INPUT_MAX_LEN + 1]; /* NULL-terminated text */
	int len;			/* Current length */
	int cursor;			/* Cursor position (0 to len) */
	int scroll_offset;		/* Horizontal scroll for long text */
};

/*
 * Initialize input to empty state.
 */
void ui_input_init(struct ui_input *input);

/*
 * Set text programmatically (e.g., loading saved state).
 */
void ui_input_set_text(struct ui_input *input, const char *text);

/*
 * Get current text (read-only access to buffer).
 */
const char *ui_input_get_text(struct ui_input *input);

/*
 * Handle keyboard input.
 *
 * @param input      Input state to modify
 * @param keysym     XKB keysym (e.g., XKB_KEY_a, XKB_KEY_Left)
 * @param mods       Modifier flags (MOD_CTRL, MOD_ALT, etc.)
 * @param codepoint  UTF-32 codepoint for printable chars, 0 otherwise
 * @return           true if state changed (needs redraw)
 *
 * Returns false for keys it doesn't handle, allowing the caller
 * to process global shortcuts (Escape, Ctrl-Q, etc.).
 */
bool ui_input_handle_key(struct ui_input *input,
			 uint32_t keysym,
			 uint32_t mods,
			 uint32_t codepoint);

#endif /* UI_INPUT_H */
