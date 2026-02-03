#include "ui_label.h"

#include <stdlib.h>

#include "../platform/platform.h"
#include "../render/render_font.h"

void
ui_label_draw(
    struct ui_ctx *ctx, int x, int y, str text, enum ui_label_style style)
{
	uint32_t color;

	switch (style) {
	case UI_LABEL_SECONDARY:
		color = ctx->theme.fg_secondary;
		break;
	case UI_LABEL_MUTED:
		color = ctx->theme.fg_muted;
		break;
	case UI_LABEL_ACCENT:
		color = ctx->theme.accent;
		break;
	case UI_LABEL_NORMAL:
	default:
		color = ctx->theme.fg_primary;
		break;
	}

	ui_label_draw_colored(ctx, x, y, text, color);
}

void
ui_label_draw_colored(
    struct ui_ctx *ctx, int x, int y, str text, uint32_t color)
{
	int baseline_y;
	char *cstr;

	/* Convert top-left y to baseline y */
	baseline_y = y + font_get_ascent(ctx->font);

	cstr = str_to_cstr(text);
	font_draw_text(ctx->font,
		       ctx->fb->pixels,
		       ctx->fb->width,
		       ctx->fb->height,
		       x,
		       baseline_y,
		       cstr,
		       color);
	free(cstr);
}

int
ui_label_width(struct ui_ctx *ctx, str text)
{
	char *cstr;
	int width;

	cstr = str_to_cstr(text);
	width = font_measure_text(ctx->font, cstr, NULL);
	free(cstr);
	return width;
}

int
ui_label_height(struct ui_ctx *ctx)
{
	return font_get_line_height(ctx->font);
}
