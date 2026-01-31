#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui_types.h"

/*
 * Button state flags.
 * Can be combined with bitwise OR.
 */
enum ui_button_state {
	UI_BTN_NORMAL = 0,
	UI_BTN_FOCUSED = (1 << 0),  /* Has keyboard focus */
	UI_BTN_ACTIVE = (1 << 1),   /* Toggled on / selected */
	UI_BTN_DISABLED = (1 << 2), /* Cannot be interacted with */
};

/*
 * Button configuration.
 * User fills this out, passes to draw function.
 */
struct ui_button_cfg {
	const char *label; /* Button text */
	const char
	    *status_text; /* Optional right-aligned status (e.g. "[ON]") */
	int state;	  /* Combination of ui_button_state flags */
	int focus_indicator_width; /* Width of left focus bar (0 to disable) */
	int padding_x;		   /* Horizontal text padding */
};

/*
 * Draw a button.
 *
 * @param ctx    Rendering context
 * @param rect   Button bounds
 * @param cfg    Button configuration
 *
 * Visual behavior:
 * - Normal: dark background
 * - Focused: lighter background, blue focus ring, left indicator bar
 * - Active: green-tinted background
 * - Active+Focused: brighter green background
 */
void ui_button_draw(struct ui_ctx *ctx,
		    struct ui_rect rect,
		    struct ui_button_cfg *cfg);

/*
 * Get default button configuration.
 * Sets reasonable defaults for focus indicator, padding, etc.
 */
struct ui_button_cfg ui_button_cfg_default(const char *label);

#endif /* UI_BUTTON_H */
