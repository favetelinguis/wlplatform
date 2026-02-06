/* include/render/render_primitives.h
 *
 * Drawing primitives using render_ctx.
 * Layer 1 - depends on core/, render_types only.
 */

#ifndef RENDER_PRIMITIVES_H
#define RENDER_PRIMITIVES_H

#include <render/render_types.h>
#include <core/str.h>

void draw_rect(struct render_ctx *ctx, struct render_rect r, uint32_t color);
void draw_rect_outline(struct render_ctx *ctx,
		       struct render_rect r,
		       uint32_t color,
		       int thickness);
void draw_text(struct render_ctx *ctx,
	       int x,
	       int y,
	       struct str text,
	       uint32_t color);

#endif /* RENDER_PRIMITIVES_H */
