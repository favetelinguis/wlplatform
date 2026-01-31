#include "ui_ctx.h"

#include "../render/render_font.h"

struct ui_theme
ui_theme_zenburn(void)
{
	struct ui_theme t;

	/*
	 * Zenburn color scheme by Jani Nurminen
	 * A low-contrast theme that's easy on the eyes for long coding
	 * sessions. https://kippura.org/zenburnpage/
	 */
	t.bg_primary = 0xFF3F3F3F;	/* Main background */
	t.bg_secondary = 0xFF4F4F4F;	/* Widget/panel background */
	t.bg_hover = 0xFF5F5F5F;	/* Focused/hover state */
	t.bg_active = 0xFF5F7F5F;	/* Active/selected (green tint) */
	t.bg_active_hover = 0xFF6F8F6F; /* Active + focused */

	t.fg_primary = 0xFFDCDCCC;   /* Zenburn foreground */
	t.fg_secondary = 0xFFC0C0C0; /* Slightly dimmer text */
	t.fg_muted = 0xFF6F6F6F;     /* Hint/disabled text */

	t.accent = 0xFFF0DFAF;	/* Zenburn yellow - focus rings */
	t.success = 0xFF7F9F7F; /* Zenburn green - success/on state */

	return t;
}

void
ui_ctx_init(struct ui_ctx *ctx, struct framebuffer *fb, struct font_ctx *font)
{
	ctx->fb = fb;
	ctx->font = font;
	ctx->theme = ui_theme_zenburn();
}

void
ui_ctx_clear(struct ui_ctx *ctx)
{
	int i;
	int total = ctx->fb->width * ctx->fb->height;

	for (i = 0; i < total; i++) {
		ctx->fb->pixels[i] = ctx->theme.bg_primary;
	}
}
