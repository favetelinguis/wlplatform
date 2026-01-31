/* src/ui/ui_types.h */

#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Rectangle for layout.
 * Using int for coordinates (sufficient for UI, avoids float complexity).
 */
struct ui_rect {
	int x, y;
	int w, h;
};

/*
 * Color theme for consistent styling.
 * All colors in XRGB8888 format.
 * Based on Zenburn color scheme (low-contrast, easy on the eyes).
 */
struct ui_theme {
	uint32_t bg_primary;   /* 0xFF3F3F3F - Zenburn background */
	uint32_t bg_secondary; /* 0xFF4F4F4F - widget background */
	uint32_t bg_hover;     /* 0xFF5F5F5F - focused/hover state */
	uint32_t bg_active;    /* 0xFF5F7F5F - activated state (green tint) */
	uint32_t bg_active_hover; /* 0xFF6F8F6F - activated + focused */

	uint32_t fg_primary;   /* 0xFFDCDCCC - Zenburn foreground */
	uint32_t fg_secondary; /* 0xFFC0C0C0 - secondary text */
	uint32_t fg_muted;     /* 0xFF6F6F6F - disabled/hint text */

	uint32_t accent;  /* 0xFFF0DFAF - Zenburn yellow (focus) */
	uint32_t success; /* 0xFF7F9F7F - Zenburn green */
};

/*
 * Rendering context passed to all draw functions.
 * Avoids threading framebuffer/font through every call.
 */
struct ui_ctx {
	struct framebuffer *fb;
	struct font_ctx *font;
	struct ui_theme theme;
};

#endif /* UI_TYPES_H */
