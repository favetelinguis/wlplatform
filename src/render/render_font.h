/* src/render/render_font.h
 *
 * Font loading and text rendering API.
 *
 * Uses stb_truetype to load fonts and render glyphs into a cached atlas.
 * The atlas is a single grayscale image containing all rendered glyphs.
 *
 * DESIGN: Simple, immediate-mode style API. ASCII-only for simplicity.
 * Thread-safe if each thread has its own font_ctx.
 */

#ifndef RENDER_FONT_H
#define RENDER_FONT_H

#include <core/str.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Opaque font context.
 * Contains stb_truetype state and glyph cache.
 */
struct font_ctx;

/*
 * Text measurement result.
 */
struct text_metrics {
	int width;   /* Total width in pixels */
	int height;  /* Line height in pixels */
	int ascent;  /* Distance from baseline to top */
	int descent; /* Distance from baseline to bottom (positive) */
};

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

/*
 * Create a font context and load a font file.
 *
 * @param path      Path to TTF file
 * @param size_px   Font size in pixels (height)
 * @return          Font context, or NULL on failure
 *
 * Example:
 *   struct font_ctx *font =
 *       font_create("assets/fonts/JetBrainsMono-Regular.ttf", 16);
 */
struct font_ctx *font_create(const char *path, int size_px);

/*
 * Destroy font context and free resources.
 * Safe to call with NULL.
 */
void font_destroy(struct font_ctx *font);

/* ============================================================
 * TEXT RENDERING
 * ============================================================ */

/*
 * Draw ASCII text to a framebuffer.
 *
 * @param font      Font context
 * @param pixels    Destination XRGB8888 pixel buffer
 * @param fb_width  Framebuffer width (for stride calculation)
 * @param fb_height Framebuffer height (for clipping)
 * @param x         X position (left edge of first character)
 * @param y         Y position (baseline)
 * @param text      ASCII text to render (non-ASCII chars skipped)
 * @param color     Text color in XRGB8888 format (0xAARRGGBB, AA ignored)
 *
 * Note: y is the BASELINE position, not top of text.
 * Use font_get_ascent() to convert from top-left positioning.
 */
void font_draw_text(struct font_ctx *font,
		    uint32_t *pixels,
		    int fb_width,
		    int fb_height,
		    int x,
		    int y,
		    struct str text,
		    uint32_t color);

/*
 * Draw text with selection highlighting.
 *
 * @param sel_start  Start index of selection (character index)
 * @param sel_end    End index of selection (exclusive)
 * @param sel_color  Background color for selection
 *
 * Characters from sel_start to sel_end-1 will have sel_color background.
 */
void font_draw_text_selected(struct font_ctx *font,
			     uint32_t *pixels,
			     int fb_width,
			     int fb_height,
			     int x,
			     int y,
			     struct str text,
			     uint32_t color,
			     int sel_start,
			     int sel_end,
			     uint32_t sel_color);

/* ============================================================
 * TEXT MEASUREMENT
 * ============================================================ */

/*
 * Measure text dimensions without rendering.
 *
 * @param font    Font context
 * @param text    ASCII text to measure
 * @param out     Output metrics (can be NULL if only width needed)
 * @return        Width in pixels
 */
int font_measure_text(struct font_ctx *font,
		      struct str text,
		      struct text_metrics *out);

/*
 * Get X position of character at given index.
 * Useful for cursor positioning.
 *
 * @param font    Font context
 * @param text    ASCII text
 * @param index   Character index into text
 * @return        X offset from start of text
 */
int font_char_index_to_x(struct font_ctx *font, struct str text, int index);

/*
 * Get character index at given X position.
 * Useful for click-to-cursor (if we ever add mouse support).
 *
 * @param font    Font context
 * @param text    ASCII text
 * @param x       X offset from start of text
 * @return        Character index into text
 */
int font_x_to_char_index(struct font_ctx *font, struct str text, int x);

/* ============================================================
 * FONT METRICS
 * ============================================================ */

/*
 * Get font line height (ascent + descent + line gap).
 */
int font_get_line_height(struct font_ctx *font);

/*
 * Get font ascent (baseline to top of tallest glyph).
 */
int font_get_ascent(struct font_ctx *font);

/*
 * Get font descent (baseline to bottom of lowest glyph, positive value).
 */
int font_get_descent(struct font_ctx *font);

/*
 * Get font size in pixels.
 */
int font_get_size(struct font_ctx *font);

#endif /* RENDER_FONT_H */
