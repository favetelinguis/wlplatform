/* src/render/render_font.c
 *
 * Font loading and text rendering implementation using stb_truetype.
 * ASCII-only for simplicity.
 */

#include "render_font.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../vendor/stb/stb_truetype.h"

/* ============================================================
 * CONSTANTS
 * ============================================================ */

#define ATLAS_WIDTH   512
#define ATLAS_HEIGHT  512
#define GLYPH_PADDING 1

/* Cache ASCII printable characters (32-126) */
#define CACHE_START 32
#define CACHE_END   127
#define CACHE_SIZE  (CACHE_END - CACHE_START)

/* ============================================================
 * GLYPH CACHE STRUCTURES
 * ============================================================ */

/*
 * Cached glyph information.
 */
struct glyph_info {
	int atlas_x;   /* X position in atlas */
	int atlas_y;   /* Y position in atlas */
	int width;     /* Glyph bitmap width */
	int height;    /* Glyph bitmap height */
	int bearing_x; /* Horizontal bearing (left side bearing) */
	int bearing_y; /* Vertical bearing (from baseline to top) */
	int advance_x; /* Horizontal advance */
	bool cached;   /* Has this glyph been rendered? */
};

/*
 * Glyph atlas - contains rendered glyphs.
 */
struct glyph_atlas {
	uint8_t *pixels; /* Grayscale bitmap */
	int width;
	int height;
	int cursor_x;	/* Next free X position */
	int cursor_y;	/* Current row Y position */
	int row_height; /* Height of current row */
};

/*
 * Font context - all font state.
 */
struct font_ctx {
	/* stb_truetype state */
	stbtt_fontinfo info;
	unsigned char *font_data; /* Raw TTF file data */
	float scale;		  /* Pixels per em unit */

	/* Font metrics */
	int size_px;
	int ascent;
	int descent;
	int line_gap;
	int line_height;

	/* Glyph cache */
	struct glyph_info cache[CACHE_SIZE];
	struct glyph_atlas atlas;
};

/* ============================================================
 * FILE LOADING
 * ============================================================ */
static unsigned char *
load_file(const char *path, size_t *out_size)
{
	FILE *f;
	unsigned char *data;
	long size;

	f = fopen(path, "rb");
	if (!f) {
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size <= 0) {
		fclose(f);
		return NULL;
	}
	data = malloc(size);
	if (!data) {
		fclose(f);
		return NULL;
	}

	if (fread(data, 1, size, f) != (size_t)size) {
		free(data);
		fclose(f);
		return NULL;
	}
	fclose(f);

	if (out_size) {
		*out_size = (size_t)size;
	}
	return data;
}

/* ============================================================
 * ATLAS MANAGEMENT
 * ============================================================ */

static bool
atlas_init(struct glyph_atlas *atlas)
{
	atlas->width = ATLAS_WIDTH;
	atlas->height = ATLAS_HEIGHT;
	atlas->cursor_x = GLYPH_PADDING;
	atlas->cursor_y = GLYPH_PADDING;
	atlas->row_height = 0;

	atlas->pixels = calloc(atlas->width * atlas->height, sizeof(uint8_t));
	if (!atlas->pixels) {
		return false;
	}
	return true;
}

static void
atlas_destroy(struct glyph_atlas *atlas)
{
	free(atlas->pixels);
	atlas->pixels = NULL;
}

/*
 * Allocate space in atlas for a glyph.
 * Returns false if atlas is full.
 */
static bool
atlas_allocate(
    struct glyph_atlas *atlas, int width, int height, int *out_x, int *out_y)
{
	/* Check if glyph fits in current row */
	if (atlas->cursor_x + width + GLYPH_PADDING > atlas->width) {
		/* Move to next row */
		atlas->cursor_x = GLYPH_PADDING;
		atlas->cursor_y += atlas->row_height + GLYPH_PADDING;
		atlas->row_height = 0;
	}

	/* Check if we exceeded atlas height */
	if (atlas->cursor_y + height + GLYPH_PADDING > atlas->height) {
		fprintf(stderr, "Glyph atlas full!\n");
		return false;
	}

	*out_x = atlas->cursor_x;
	*out_y = atlas->cursor_y;

	atlas->cursor_x += width + GLYPH_PADDING;
	if (height > atlas->row_height) {
		atlas->row_height = height;
	}
	return true;
}

/* ============================================================
 * GLYPH CACHING
 * ============================================================ */

/*
 * Get glyph info, rendering and caching if necessary.
 * Returns NULL if glyph cannot be rendered.
 */
static struct glyph_info *
get_glyph(struct font_ctx *font, int codepoint)
{
	struct glyph_info *glyph;
	int advance, lsb;
	int x0, y0, x1, y1;
	int width, height;
	int atlas_x, atlas_y;
	unsigned char *bitmap;

	/* Check if in ASCII cache range */
	if (codepoint < CACHE_START || codepoint >= CACHE_END) {
		/* Non-ASCII: return space as fallback */
		return &font->cache[' ' - CACHE_START];
	}

	glyph = &font->cache[codepoint - CACHE_START];
	if (glyph->cached) {
		return glyph;
	}

	/* Get glyph metrics */
	stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &lsb);
	stbtt_GetCodepointBitmapBox(&font->info,
				    codepoint,
				    font->scale,
				    font->scale,
				    &x0,
				    &y0,
				    &x1,
				    &y1);
	width = x1 - x0;
	height = y1 - y0;

	/* Handle empty glyphs (like space) */
	if (width <= 0 || height <= 0) {
		glyph->atlas_x = 0;
		glyph->atlas_y = 0;
		glyph->width = 0;
		glyph->height = 0;
		glyph->bearing_x = (int)(lsb * font->scale);
		glyph->bearing_y = 0;
		glyph->advance_x = (int)(advance * font->scale);
		glyph->cached = true;
		return glyph;
	}

	/* Allocate space in atlas */
	if (!atlas_allocate(&font->atlas, width, height, &atlas_x, &atlas_y)) {
		return NULL;
	}

	/* Render glyph directrly into atlas */
	bitmap = font->atlas.pixels + atlas_y * font->atlas.width + atlas_x;
	stbtt_MakeCodepointBitmap(&font->info,
				  bitmap,
				  width,
				  height,
				  font->atlas.width, /* stride */
				  font->scale,
				  font->scale,
				  codepoint);

	/* Store glyph info */
	glyph->atlas_x = atlas_x;
	glyph->atlas_y = atlas_y;
	glyph->width = width;
	glyph->height = height;
	glyph->bearing_x = (int)(lsb * font->scale);
	glyph->bearing_y =
	    -y0; /* Convert from top-relative to baseline-relative */
	glyph->advance_x = (int)(advance * font->scale);
	glyph->cached = true;

	return glyph;
}

/* ============================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================ */

struct font_ctx *
font_create(const char *path, int size_px)
{
	struct font_ctx *font;
	int ascent, descent, line_gap;
	int c;

	font = calloc(1, sizeof(*font));
	if (!font) {
		return NULL;
	}

	font->size_px = size_px;

	/* Load font file */
	font->font_data = load_file(path, NULL);
	if (!font->font_data) {
		fprintf(stderr, "Failed to load font file '%s'\n", path);
		free(font);
		return NULL;
	}

	/* Initialize stb_truetype */
	if (!stbtt_InitFont(&font->info,
			    font->font_data,
			    stbtt_GetFontOffsetForIndex(font->font_data, 0))) {
		fprintf(stderr, "Failed to initialize font '%s'\n", path);
		free(font->font_data);
		free(font);
		return NULL;
	}

	/* Calculate scale factor */
	font->scale = stbtt_ScaleForPixelHeight(&font->info, (float)size_px);

	/* Get font metrics */
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
	font->ascent = (int)(ascent * font->scale);
	font->descent = (int)(-descent * font->scale); /* Make positive */
	font->line_gap = (int)(line_gap * font->scale);
	font->line_height = font->ascent + font->descent + font->line_gap;

	/* Initialize glyph atlas */
	if (!atlas_init(&font->atlas)) {
		free(font->font_data);
		free(font);
		return NULL;
	}

	/* Pre-cach ASCII characters */
	for (c = CACHE_START; c < CACHE_END; c++) {
		get_glyph(font, c);
	}

	return font;
}

void
font_destroy(struct font_ctx *font)
{
	if (!font) {
		return;
	}

	atlas_destroy(&font->atlas);
	free(font->font_data);
	free(font);
}

/*
 * Alpha blend a pixel.
 * src_alpha: 0-255 coverage from glyph
 * fg_color: text color (XRGB8888)
 * bg_pixel: existing pixel in framebuffer
 */
static uint32_t
blend_pixel(uint8_t src_alpha, uint32_t fg_color, uint32_t bg_pixel)
{
	uint32_t fg_r, fg_g, fg_b;
	uint32_t bg_r, bg_g, bg_b;
	uint32_t out_r, out_g, out_b;
	uint32_t alpha, inv_alpha;

	if (src_alpha == 0) {
		return bg_pixel;
	}
	if (src_alpha == 255) {
		return fg_color;
	}

	alpha = src_alpha;
	inv_alpha = 255 - alpha;

	fg_r = (fg_color >> 16) & 0xFF;
	fg_g = (fg_color >> 8) & 0xFF;
	fg_b = fg_color & 0xFF;

	bg_r = (bg_pixel >> 16) & 0xFF;
	bg_g = (bg_pixel >> 8) & 0xFF;
	bg_b = bg_pixel & 0xFF;

	out_r = (fg_r * alpha + bg_r * inv_alpha) / 255;
	out_g = (fg_g * alpha + bg_g * inv_alpha) / 255;
	out_b = (fg_b * alpha + bg_b * inv_alpha) / 255;

	return 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
}

void
font_draw_text(struct font_ctx *font,
	       uint32_t *pixels,
	       int fb_width,
	       int fb_height,
	       int x,
	       int y,
	       const char *text,
	       uint32_t color)
{
	int pen_x;
	int i;

	if (!font || !pixels || !text) {
		return;
	}

	pen_x = x;

	for (i = 0; text[i] != '\0'; i++) {
		struct glyph_info *glyph;
		int draw_x, draw_y;
		int row, col;
		int c;

		c = (unsigned char)text[i];

		glyph = get_glyph(font, c);
		if (!glyph) {
			continue;
		}

		/* Calculate draw position */
		draw_x = pen_x + glyph->bearing_x;
		draw_y = y - glyph->bearing_y;

		/* Draw glyph from atlas to framebuffer */
		for (row = 0; row < glyph->height; row++) {
			int gy = draw_y + row;
			if (gy < 0 || gy >= fb_height) {
				continue;
			}

			for (col = 0; col < glyph->width; col++) {
				int gx = draw_x + col;
				int atlas_idx;
				int fb_idx;
				uint8_t alpha;

				if (gx < 0 || gx >= fb_width) {
					continue;
				}

				atlas_idx = (glyph->atlas_y + row) *
						font->atlas.width +
					    (glyph->atlas_x + col);
				fb_idx = gy * fb_width + gx;

				alpha = font->atlas.pixels[atlas_idx];
				pixels[fb_idx] =
				    blend_pixel(alpha, color, pixels[fb_idx]);
			}
		}

		pen_x += glyph->advance_x;
	}
}

void
font_draw_text_selected(struct font_ctx *font,
			uint32_t *pixels,
			int fb_width,
			int fb_height,
			int x,
			int y,
			const char *text,
			uint32_t color,
			int sel_start,
			int sel_end,
			uint32_t sel_color)
{
	int pen_x;
	int i;

	if (!font || !pixels || !text) {
		return;
	}

	pen_x = x;

	for (i = 0; text[i] != '\0'; i++) {
		struct glyph_info *glyph;
		int char_start_x;
		bool in_selection;
		int c;

		c = (unsigned char)text[i];
		char_start_x = pen_x;

		glyph = get_glyph(font, c);
		if (!glyph) {
			continue;
		}

		/* Check if this character is in selection */
		in_selection = (i >= sel_start && i < sel_end);

		/* Draw selection background */
		if (in_selection) {
			int sel_x = char_start_x;
			int sel_y = y - font->ascent;
			int sel_w = glyph->advance_x;
			int sel_h = font->line_height;
			int row, col;

			for (row = 0; row < sel_h; row++) {
				int gy = sel_y + row;
				if (gy < 0 || gy >= fb_height) {
					continue;
				}
				for (col = 0; col < sel_w; col++) {
					int gx = sel_x + col;
					if (gx < 0 || gx >= fb_width) {
						continue;
					}
					pixels[gy * fb_width + gx] = sel_color;
				}
			}
		}

		/* Draw glyph */
		{
			int draw_x = pen_x + glyph->bearing_x;
			int draw_y = y - glyph->bearing_y;
			int row, col;

			for (row = 0; row < glyph->height; row++) {
				int gy = draw_y + row;
				if (gy < 0 || gy >= fb_height) {
					continue;
				}
				for (col = 0; col < glyph->width; col++) {
					int gx = draw_x + col;
					int atlas_idx;
					int fb_idx;
					uint8_t alpha;

					if (gx < 0 || gx >= fb_width) {
						continue;
					}

					atlas_idx = (glyph->atlas_y + row) *
							font->atlas.width +
						    (glyph->atlas_x + col);
					fb_idx = gy * fb_width + gx;

					alpha = font->atlas.pixels[atlas_idx];
					pixels[fb_idx] = blend_pixel(
					    alpha, color, pixels[fb_idx]);
				}
			}
		}

		pen_x += glyph->advance_x;
	}
}

int
font_measure_text(struct font_ctx *font,
		  const char *text,
		  struct text_metrics *out)
{
	int width;
	int i;

	if (!font || !text) {
		return 0;
	}

	width = 0;

	for (i = 0; text[i] != '\0'; i++) {
		struct glyph_info *glyph;
		int c;

		c = (unsigned char)text[i];
		glyph = get_glyph(font, c);
		if (glyph) {
			width += glyph->advance_x;
		}
	}

	if (out) {
		out->width = width;
		out->height = font->line_height;
		out->ascent = font->ascent;
		out->descent = font->descent;
	}

	return width;
}

int
font_char_index_to_x(struct font_ctx *font, const char *text, int index)
{
	int x;
	int i;

	if (!font || !text || index <= 0) {
		return 0;
	}

	x = 0;

	for (i = 0; text[i] != '\0' && i < index; i++) {
		struct glyph_info *glyph;
		int c;

		c = (unsigned char)text[i];
		glyph = get_glyph(font, c);
		if (glyph) {
			x += glyph->advance_x;
		}
	}

	return x;
}

int
font_x_to_char_index(struct font_ctx *font, const char *text, int target_x)
{
	int x;
	int prev_x;
	int i;

	if (!font || !text || target_x <= 0) {
		return 0;
	}

	x = 0;
	prev_x = 0;

	for (i = 0; text[i] != '\0'; i++) {
		struct glyph_info *glyph;
		int c;

		c = (unsigned char)text[i];
		glyph = get_glyph(font, c);
		if (glyph) {
			x += glyph->advance_x;
		}

		/* Check if target_x is closer to previous or current position
		 */
		if (x > target_x) {
			if (target_x - prev_x < x - target_x) {
				return i;
			} else {
				return i + 1;
			}
		}

		prev_x = x;
	}

	return i;
}

int
font_get_line_height(struct font_ctx *font)
{
	return font ? font->line_height : 0;
}

int
font_get_ascent(struct font_ctx *font)
{
	return font ? font->ascent : 0;
}

int
font_get_descent(struct font_ctx *font)
{
	return font ? font->descent : 0;
}

int
font_get_size(struct font_ctx *font)
{
	return font ? font->size_px : 0;
}
