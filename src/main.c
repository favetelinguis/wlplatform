/* main.c
 *
 * Single-line input demo.
 *
 * Controls:
 *   Standard readline bindings for text editing
 *   Escape / Ctrl+Q - Quit
 */

#include <stdint.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "platform/platform.h"
#include "render/render_font.h"
#include "render/render_primitives.h"
#include "ui/ui.h"

/* ============================================================
 * APPLICATION STATE
 * ============================================================ */

struct app_state {
	bool running;
	bool needs_redraw;
	struct ui_input input;
	struct font_ctx *font;
};

/* ============================================================
 * INPUT HANDLING
 * ============================================================ */

static bool
handle_key(struct app_state *app, struct platform_event *ev)
{
	uint32_t key = ev->key.keysym;
	uint32_t mods = ev->key.modifiers;
	uint32_t codepoint = ev->key.codepoint;

	/*
	 * Step 1: Let input component try to handle the key.
	 */
	if (ui_input_handle_key(&app->input, key, mods, codepoint)) {
		return true;
	}

	/*
	 * Step 2: Global keys (not handled by input).
	 */
	switch (key) {
	case XKB_KEY_Escape:
		app->running = false;
		return false;

	case XKB_KEY_q:
		if (mods & MOD_CTRL) {
			app->running = false;
			return false;
		}
		break;

	case XKB_KEY_Return:
		/* Could submit the input here */
		printf("Submitted: %s\n", ui_input_get_text(&app->input));
		return false;
	}

	return false;
}

/* ============================================================
 * RENDERING
 * ============================================================ */

static void
render(struct platform *p, struct app_state *app)
{
	struct framebuffer *fb;
	struct ui_ctx ctx;
	int line_height, text_y, cursor_x;
	int padding_x = 20;

	fb = platform_get_framebuffer(p);
	if (!fb)
		return;

	ui_ctx_init(&ctx, fb, app->font);
	ui_ctx_clear(&ctx);

	line_height = font_get_line_height(app->font);

	/*
	 * Center the input line vertically.
	 * Full width minus padding on each side.
	 */
	text_y = (fb->height - line_height) / 2;

	/* Draw the text */
	ui_label_draw_colored(
	    &ctx, padding_x, text_y, app->input.buf, ctx.theme.fg_primary);

	/* Draw cursor (always visible, always focused) */
	cursor_x =
	    padding_x +
	    font_char_index_to_x(ctx.font, app->input.buf, app->input.cursor);
	{
		struct ui_rect cursor_rect = {
		    cursor_x, text_y, 2, line_height};
		draw_rect(&ctx, cursor_rect, ctx.theme.accent);
	}

	platform_present(p);
}

/* ============================================================
 * MAIN
 * ============================================================ */

int
main(int argc, char *argv[])
{
	struct platform *platform;
	struct app_state app = {0};

	(void)argc;
	(void)argv;

	/* Load font */
	app.font = font_create("assets/fonts/JetBrainsMono-Regular.ttf", 16);
	if (!app.font) {
		fprintf(stderr, "Failed to load font\n");
		return 1;
	}

	/* Initialize input */
	ui_input_init(&app.input);
	app.running = true;
	app.needs_redraw = true;

	/* Create window */
	platform = platform_create("Input Demo", 800, 600);
	if (!platform) {
		fprintf(stderr, "Failed to create platform\n");
		font_destroy(app.font);
		return 1;
	}

	printf("=== Single-Line Input Demo ===\n");
	printf("Type text. Readline shortcuts work.\n");
	printf("Enter to submit, Escape to quit.\n\n");

	/* Main loop */
	while (app.running) {
		struct platform_event ev;

		if (app.needs_redraw) {
			render(platform, &app);
			app.needs_redraw = false;
		}

		if (!platform_wait_events(platform, -1))
			break;

		while (platform_next_event(platform, &ev)) {
			switch (ev.type) {
			case EVENT_QUIT:
				app.running = false;
				break;
			case EVENT_KEY_PRESS:
				if (handle_key(&app, &ev))
					app.needs_redraw = true;
				break;
			case EVENT_RESIZE:
				app.needs_redraw = true;
				break;
			default:
				break;
			}
		}
	}

	platform_destroy(platform);
	font_destroy(app.font);
	printf("Clean shutdown\n");

	return 0;
}
