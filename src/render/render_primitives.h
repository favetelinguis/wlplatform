#ifndef RENDER_PRIMITIVES_H
#define RENDER_PRIMITIVES_H

#include <stdint.h>

#include "../ui/ui.h"

void draw_rect(struct ui_ctx *ctx, struct ui_rect r, uint32_t color);

void draw_rect_outline(struct ui_ctx *ctx,
		       struct ui_rect r,
		       uint32_t color,
		       int thickness);

void draw_text(struct ui_ctx *ctx,
	       struct font_ctx *font,
	       int x,
	       int y,
	       struct str text,
	       uint32_t color);

#endif /* RENDER_PRIMITIVES_H */
