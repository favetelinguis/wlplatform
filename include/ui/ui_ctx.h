/* include/ui/ui_ctx.h
 *
 * UI rendering context lifecycle.
 */

#ifndef UI_CTX_H
#define UI_CTX_H

#include <ui/ui_types.h>

struct ui_theme ui_theme_zenburn(void);
void ui_ctx_init(struct ui_ctx *ctx, struct framebuffer *fb, struct font_ctx *font);
void ui_ctx_clear(struct ui_ctx *ctx);

#endif /* UI_CTX_H */
