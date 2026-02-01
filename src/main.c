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
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "buffer/buffer.h"
#include "platform/platform.h"
#include "render/render_font.h"
#include "render/render_primitives.h"
#include "ui/ui.h"
#include "ui/ui_panel.h"

#define MENU_ROWS 15

/* ============================================================
 * APPLICATION STATE
 * ============================================================ */

struct app_state {
	bool running;
	bool needs_redraw;
	struct ui_input input;
	struct buffer buffer;
	struct font_ctx *font;
};

/* ============================================================
 * BUFFER HANDLING
 * ============================================================ */

/*
 * Sync input display to current buffer line.
 * Called after cursor movement.
 */
static void
sync_input_to_buffer(struct app_state *app)
{
	const char *line;

	line = buffer_get_current_line(&app->buffer);
	ui_input_set_text(&app->input, line ? line : "");
}

/* ============================================================
 * INPUT HANDLING
 * ============================================================ */

static bool
handle_key(struct app_state *app, struct platform_event *ev)
{
	uint32_t key = ev->key.keysym;
	uint32_t mods = ev->key.modifiers;
	uint32_t codepoint = ev->key.codepoint;
	int visible_lines;

	/* Step 1: Buffer navigation keys (Ctrl-N, Ctrl-P).
	 * These take priority over input handling.
	 */
	if (mods & MOD_CTRL) {
		switch (key) {
		case XKB_KEY_n:
			buffer_move_down(&app->buffer, 1);
			sync_input_to_buffer(app);
			return true;
		case XKB_KEY_p:
			buffer_move_up(&app->buffer, 1);
			sync_input_to_buffer(app);
			return true;
		}
	}

	/*
	 * Step 2: Let input component try to handle the key.
	 */
	if (ui_input_handle_key(&app->input, key, mods, codepoint)) {
		return true;
	}

	/*
	 * Step 3: Global keys.
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
	int line_height;
	int padding_x = 8;
	int input_y, input_h;
	int menu_h;
	int lines_above, lines_below;
	int y, i, line_num;
	const char *line;

	fb = platform_get_framebuffer(p);
	if (!fb)
		return;

	ui_ctx_init(&ctx, fb, app->font);
	ui_ctx_clear(&ctx);

	line_height = font_get_line_height(app->font);
	menu_h = MENU_ROWS * line_height;

	/*
	 * Calculate input box position (vertically ceneted).
	 * This is the focal point - everything else is relative to it.
	 */
	input_h = line_height + 4; /* Text height + small padding */
	input_y = (fb->height - input_h) / 2;

	/*
	 * Calculate how many context lines fit above and below.
	 */
	lines_above = input_y / line_height;
	lines_below = (fb->height - input_y - input_h - menu_h) / line_height;

	/*
	 * Draw lines ABOVE the input box (previous lines in buffer).
	 * We draw from top of screen down to just above input.
	 */
	for (i = 0; i < lines_above; i++) {
		/* Line number relative to cursor */
		line_num = app->buffer.cursor_line - (lines_above - i);
		if (line_num < 0)
			continue; /* Before start of file, leave blank */

		line = buffer_get_line(&app->buffer, line_num);
		if (!line)
			continue;

		y = i * line_height;
		ui_label_draw_colored(
		    &ctx, padding_x, y, line, ctx.theme.fg_secondary);
	}

	/*
	 * Draw INPUT BOX (current line, always centerd).
	 * This is where the cursor lives.
	 */
	{
		int cursor_x;

		/* Background hightlight for input area */
		struct ui_rect input_bg = {0, input_y, fb->width, input_h};
		draw_rect(&ctx, input_bg, ctx.theme.bg_hover);

		/* Text (vertically centered in input box) */
		int text_y = input_y + (input_h - line_height) / 2;
		ui_label_draw_colored(&ctx,
				      padding_x,
				      text_y,
				      app->input.buf,
				      ctx.theme.fg_primary);

		/* Cursor */
		cursor_x = padding_x + font_char_index_to_x(ctx.font,
							    app->input.buf,
							    app->input.cursor);
		struct ui_rect cursor_rect = {
		    cursor_x, text_y, 2, line_height};
		draw_rect(&ctx, cursor_rect, ctx.theme.accent);
	}

	/*
	 * Draw lines BELOW the input box (next lines in buffer).
	 */
	for (i = 0; i < lines_below; i++) {
		line_num = app->buffer.cursor_line + 1 + i;
		if (line_num >= app->buffer.line_count)
			break; /* Past end of file */

		line = buffer_get_line(&app->buffer, line_num);
		if (!line)
			continue;

		y = input_y + input_h + (i * line_height);
		ui_label_draw_colored(
		    &ctx, padding_x, y, line, ctx.theme.fg_secondary);
	}

	/* Menu area */
	struct ui_rect menu_rect = {0, fb->height - menu_h, fb->width, menu_h};
	ui_panel_draw(&ctx, menu_rect, ctx.theme.bg_hover, UI_PANEL_FLAT);

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
	const char *filepath;

	/* Print PID for easy killing */
	printf("PID: %d\n", getpid());

	/* Require filename argument */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return 1;
	}
	filepath = argv[1];

	/* Initialize buffer and load file */
	buffer_init(&app.buffer);
	if (!buffer_load(&app.buffer, filepath)) {
		fprintf(stderr, "Failed to load: %s\n", filepath);
		return 1;
	}

	/* Load font */
	app.font = font_create("assets/fonts/JetBrainsMono-Regular.ttf", 20);
	if (!app.font) {
		fprintf(stderr, "Failed to load font\n");
		buffer_destroy(&app.buffer);
		return 1;
	}

	/* Initialize input with first line */
	ui_input_init(&app.input);
	ui_input_set_text(&app.input, buffer_get_current_line(&app.buffer));

	/* Basic initialization of app */
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

	/* Cleanup (reverse order of initialization) */
	platform_destroy(platform);
	font_destroy(app.font);
	buffer_destroy(&app.buffer);

	printf("Clean shutdown\n");
	return 0;
}
