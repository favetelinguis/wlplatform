#include <ui/ui_label.h>

#include <stdlib.h>

#include <render/render_font.h>

void
ui_label_draw(
    struct ui_ctx *ctx, int x, int y, struct str text, enum ui_label_style style)
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
    struct ui_ctx *ctx, int x, int y, struct str text, uint32_t color)
{
	int baseline_y;

	/* Convert top-left y to baseline y */
	baseline_y = y + font_get_ascent(ctx->render.font);

	font_draw_text(ctx->render.font,
		       ctx->render.fb->pixels,
		       ctx->render.fb->width,
		       ctx->render.fb->height,
		       x,
		       baseline_y,
		       text,
		       color);
}

int
ui_label_width(struct ui_ctx *ctx, struct str text)
{
	return font_measure_text(ctx->render.font, text, NULL);
}

int
ui_label_height(struct ui_ctx *ctx)
{
	return font_get_line_height(ctx->render.font);
}
