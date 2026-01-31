#include "render_primitives.h"

#include "../render/render_font.h"
#include "../ui/ui.h"

void
draw_rect(struct ui_ctx *ctx, struct ui_rect r, uint32_t color)
{
	int x, y;
	int x0, y0, x1, y1;
	struct framebuffer *fb = ctx->fb;

	x0 = r.x < 0 ? 0 : r.x;
	y0 = r.y < 0 ? 0 : r.y;
	x1 = (r.x + r.w) > fb->width ? fb->width : (r.x + r.w);
	y1 = (r.y + r.h) > fb->height ? fb->height : (r.y + r.h);

	for (y = y0; y < y1; y++) {
		for (x = x0; x < x1; x++) {
			fb->pixels[y * fb->width + x] = color;
		}
	}
}

void
draw_rect_outline(struct ui_ctx *ctx,
		  struct ui_rect r,
		  uint32_t color,
		  int thickness)
{
	struct ui_rect top = {r.x, r.y, r.w, thickness};
	struct ui_rect bottom = {r.x, r.y + r.h - thickness, r.w, thickness};
	struct ui_rect left = {r.x, r.y, thickness, r.h};
	struct ui_rect right = {r.x + r.w - thickness, r.y, thickness, r.h};

	draw_rect(ctx, top, color);
	draw_rect(ctx, bottom, color);
	draw_rect(ctx, left, color);
	draw_rect(ctx, right, color);
}

void
draw_text(struct ui_ctx *ctx,
	  struct font_ctx *font,
	  int x,
	  int y,
	  const char *text,
	  uint32_t color)
{
	/* y is top-left, but font_draw_text expect baseline */
	int baseline_y = y + font_get_ascent(font);
	font_draw_text(font,
		       ctx->fb->pixels,
		       ctx->fb->width,
		       ctx->fb->height,
		       x,
		       baseline_y,
		       text,
		       color);
}
