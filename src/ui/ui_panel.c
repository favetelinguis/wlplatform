/* src/ui/ui_panel.c */

#include "ui_panel.h"

#include "../render/render_primitives.h"

void
ui_panel_draw(struct ui_ctx *ctx,
	      struct ui_rect rect,
	      uint32_t color,
	      enum ui_panel_style style)
{
	/* Draw background */
	draw_rect(ctx, rect, color);

	/* Draw border if requested */
	if (style == UI_PANEL_BORDERED) {
		draw_rect_outline(ctx, rect, ctx->theme.fg_muted, 1);
	}
}

void
ui_panel_draw_default(struct ui_ctx *ctx, struct ui_rect rect)
{
	ui_panel_draw(ctx, rect, ctx->theme.bg_secondary, UI_PANEL_FLAT);
}
