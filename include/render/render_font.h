/* include/render/render_font.h
 *
 * Font loading and text rendering via stb_truetype.
 * Layer 1 - depends on core/ only.
 */

#ifndef RENDER_FONT_H
#define RENDER_FONT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <core/str.h>

struct font_ctx; /* Opaque */

struct text_metrics {
	int width;
	int height;
	int ascent;
	int descent;
};

struct font_ctx *font_create(const char *path, int size_px);
void font_destroy(struct font_ctx *font);

void font_draw_text(struct font_ctx *font,
		    uint32_t *pixels,
		    int fb_width,
		    int fb_height,
		    int x,
		    int y,
		    struct str text,
		    uint32_t color);

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

int font_measure_text(struct font_ctx *font,
		      struct str text,
		      struct text_metrics *out);

int font_char_index_to_x(struct font_ctx *font, struct str text, int index);
int font_x_to_char_index(struct font_ctx *font, struct str text, int target_x);

int font_get_line_height(struct font_ctx *font);
int font_get_ascent(struct font_ctx *font);
int font_get_descent(struct font_ctx *font);
int font_get_size(struct font_ctx *font);

#endif /* RENDER_FONT_H */
