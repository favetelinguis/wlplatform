#ifndef UI_PANEL_H
#define UI_PANEL_H

#include "ui_types.h"

/*
 * Panel style options.
 */
enum ui_panel_style {
	UI_PANEL_FLAT,	   /* Solid background, no border */
	UI_PANEL_BORDERED, /* Solid background with border */
};

/*
 * Draw a panel (colored rectangle).
 *
 * @param ctx     Rendering context
 * @param rect    Panel bounds
 * @param color   Background color
 * @param style   Border style
 */
void ui_panel_draw(struct ui_ctx *ctx,
		   struct ui_rect rect,
		   uint32_t color,
		   enum ui_panel_style style);

/*
 * Draw panel with theme's secondary background.
 */
void ui_panel_draw_default(struct ui_ctx *ctx, struct ui_rect rect);

#endif /* UI_PANEL_H */
