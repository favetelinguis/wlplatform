#include "ui_button.h"

#include <stddef.h>

#include "../render/render_primitives.h"
#include "../string/str.h"
#include "ui_label.h"

struct ui_button_cfg
ui_button_cfg_default(const char *label)
{
	struct ui_button_cfg cfg;

	cfg.label = label;
	cfg.status_text = NULL;
	cfg.state = UI_BTN_NORMAL;
	cfg.focus_indicator_width = 4;
	cfg.padding_x = 20;

	return cfg;
}

void
ui_button_draw(struct ui_ctx *ctx,
	       struct ui_rect rect,
	       struct ui_button_cfg *cfg)
{
	bool focused, active;
	uint32_t bg_color, text_color;
	int text_y, text_height;

	focused = (cfg->state & UI_BTN_FOCUSED) != 0;
	active = (cfg->state & UI_BTN_ACTIVE) != 0;

	/* Determine background color based on state */
	if (active && focused) {
		bg_color = ctx->theme.bg_active_hover;
	} else if (active) {
		bg_color = ctx->theme.bg_active;
	} else if (focused) {
		bg_color = ctx->theme.bg_hover;
	} else {
		bg_color = ctx->theme.bg_secondary;
	}

	/* Draw background */
	draw_rect(ctx, rect, bg_color);

	/* Draw focus indicator (left bar) */
	if (focused && cfg->focus_indicator_width > 0) {
		struct ui_rect indicator = {
		    rect.x, rect.y, cfg->focus_indicator_width, rect.h};
		draw_rect(ctx, indicator, ctx->theme.accent);
	}

	/* Draw focus ring */
	if (focused) {
		struct ui_rect ring = {
		    rect.x - 2, rect.y - 2, rect.w + 4, rect.h + 4};
		draw_rect_outline(ctx, ring, ctx->theme.accent, 2);
	}

	/* Draw lebel text */
	text_color = focused ? ctx->theme.fg_primary : ctx->theme.fg_secondary;
	text_height = ui_label_height(ctx);
	text_y = rect.y + (rect.h - text_height) / 2;

	ui_label_draw_colored(
	    ctx, rect.x + cfg->padding_x, text_y, str_from_cstr(cfg->label), text_color);

	/* Draw status text if present */
	if (cfg->status_text) {
		str status = str_from_cstr(cfg->status_text);
		int status_width = ui_label_width(ctx, status);
		int status_x = rect.x + rect.w - status_width - cfg->padding_x;

		ui_label_draw_colored(ctx,
				      status_x,
				      text_y,
				      status,
				      ctx->theme.success);
	}
}
