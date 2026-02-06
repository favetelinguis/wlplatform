/* include/ui/ui_button.h
 *
 * Button component.
 */

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include <ui/ui_types.h>

enum ui_button_state {
	UI_BTN_NORMAL  = 0,
	UI_BTN_FOCUSED = (1 << 0),
	UI_BTN_ACTIVE  = (1 << 1),
};

struct ui_button_cfg {
	const char *label;
	const char *status_text;
	int state;
	int focus_indicator_width;
	int padding_x;
};

struct ui_button_cfg ui_button_cfg_default(const char *label);
void ui_button_draw(struct ui_ctx *ctx, ui_rect rect, struct ui_button_cfg *cfg);

#endif /* UI_BUTTON_H */
