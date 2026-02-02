#ifndef UI_LABEL_H
#define UI_LABEL_H

#include "../string/str.h"
#include "ui_types.h"

/*
 * Text alignment options.
 */
enum ui_align {
	UI_ALIGN_LEFT,
	UI_ALIGN_CENTER,
	UI_ALIGN_RIGHT,
};

/*
 * Label style variants.
 */
enum ui_label_style {
	UI_LABEL_NORMAL,    /* Primary foreground color */
	UI_LABEL_SECONDARY, /* Secondary (dimmer) color */
	UI_LABEL_MUTED,	    /* Muted (hint text) color */
	UI_LABEL_ACCENT,    /* Accent color (highlights) */
};

/*
 * Draw a text label.
 *
 * @param ctx    Rendering context
 * @param x      X position (left edge for LEFT align)
 * @param y      Y position (top edge, NOT baseline)
 * @param text   Text to display
 * @param style  Color variant
 *
 * Note: Unlike font_draw_text(), y is top of text, not baseline.
 */
void ui_label_draw(
    struct ui_ctx *ctx, int x, int y, str text, enum ui_label_style style);

/*
 * Draw a text label with explicit color.
 */
void ui_label_draw_colored(
    struct ui_ctx *ctx, int x, int y, str text, uint32_t color);

/*
 * Measure label width without drawing.
 */
int ui_label_width(struct ui_ctx *ctx, str text);

/*
 * Get label height (line height from font).
 */
int ui_label_height(struct ui_ctx *ctx);

#endif /* UI_LABEL_H */
