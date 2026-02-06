/* include/ui/ui_label.h
 *
 * Text label component.
 */

#ifndef UI_LABEL_H
#define UI_LABEL_H

#include <core/str.h>
#include <ui/ui_types.h>

enum ui_label_style {
	UI_LABEL_NORMAL,
	UI_LABEL_SECONDARY,
	UI_LABEL_MUTED,
	UI_LABEL_ACCENT,
};

void ui_label_draw(struct ui_ctx *ctx,
		   int x,
		   int y,
		   struct str text,
		   enum ui_label_style style);

void ui_label_draw_colored(struct ui_ctx *ctx,
			   int x,
			   int y,
			   struct str text,
			   uint32_t color);

int ui_label_width(struct ui_ctx *ctx, struct str text);
int ui_label_height(struct ui_ctx *ctx);

#endif /* UI_LABEL_H */
