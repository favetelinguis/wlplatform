/* include/render/render_types.h
 *
 * Layer 1 render types - depends on core/ only.
 * Owns framebuffer, render_rect, and render_ctx.
 */

#ifndef RENDER_TYPES_H
#define RENDER_TYPES_H

#include <stdint.h>

/*
 * Framebuffer for rendering.
 * You draw into pixels[], then call platform_present().
 */
struct framebuffer {
	uint32_t *pixels; /* XRGB8888 format (0xAARRGGBB, AA ignored) */
	int width;
	int height;
	int stride; /* Bytes per row (usually width * 4) */
};

/*
 * Rectangle for rendering and layout.
 */
struct render_rect {
	int x, y;
	int w, h;
};

/*
 * Forward declaration for font context.
 */
struct font_ctx;

/*
 * Rendering context carrying framebuffer and font.
 * Passed to all render primitive functions.
 */
struct render_ctx {
	struct framebuffer *fb;
	struct font_ctx *font;
};

#endif /* RENDER_TYPES_H */
