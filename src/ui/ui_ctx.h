#ifndef UI_CTX_H
#define UI_CTX_H

#include "../platform/platform.h"
#include "ui_types.h"

/*
 * Initialize UI context with default theme.
 *
 * @param ctx    Context to initialize
 * @param fb     Framebuffer for rendering
 * @param font   Font for text rendering
 */
void
ui_ctx_init(struct ui_ctx *ctx, struct framebuffer *fb, struct font_ctx *font);

/*
 * Get default Zenburn theme.
 * Based on Zenburn color scheme by Jani Nurminen.
 */
struct ui_theme ui_theme_zenburn(void);

/*
 * Clear the framebuffer with background color.
 */
void ui_ctx_clear(struct ui_ctx *ctx);

#endif /* UI_CTX_H */
