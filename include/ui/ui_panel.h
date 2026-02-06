/* include/ui/ui_panel.h
 *
 * Panel / rectangle component.
 */

#ifndef UI_PANEL_H
#define UI_PANEL_H

#include <ui/ui_types.h>

enum ui_panel_style {
	UI_PANEL_FLAT,
	UI_PANEL_BORDERED,
};

void ui_panel_draw(struct ui_ctx *ctx,
		   ui_rect rect,
		   uint32_t color,
		   enum ui_panel_style style);

void ui_panel_draw_default(struct ui_ctx *ctx, ui_rect rect);

#endif /* UI_PANEL_H */
