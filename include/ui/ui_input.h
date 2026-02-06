/* include/ui/ui_input.h
 *
 * Single-line text input.
 */

#ifndef UI_INPUT_H
#define UI_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#define UI_INPUT_MAX_LEN 1024

struct ui_input {
	char buf[UI_INPUT_MAX_LEN + 1];
	int len;
	int cursor;
	int scroll_offset;
};

void ui_input_init(struct ui_input *input);
void ui_input_set_text(struct ui_input *input, const char *text);
const char *ui_input_get_text(struct ui_input *input);
bool ui_input_handle_key(struct ui_input *in,
			 uint32_t keysym,
			 uint32_t mods,
			 uint32_t codepoint);

#endif /* UI_INPUT_H */
