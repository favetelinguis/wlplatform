#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <core/error.h>
#include <core/str.h>
#include <editor/buffer.h>
#include <editor/syntax.h>
#include <editor/view.h>
#include <platform/platform.h>
#include <render/render_font.h>
#include <render/render_primitives.h>
#include <ui/ui.h>
#include <ui/ui_avy.h>
#include <ui/ui_menu_actions.h>
#include <ui/ui_panel.h>

#define MENU_ROWS 15

/* ============================================================
 * APPLICATION STATE
 * ============================================================ */

/*
 * Application mode state machine.
 *
 * Transitions:
 *   NORMAL --[Alt-;/Alt-']--> AVY_CHAR
 *   AVY_CHAR --[printable]--> AVY_HINT (if multiple matches)
 *   AVY_CHAR --[printable]--> AVY_ACTION (if single match)
 *   AVY_CHAR --[printable]--> NORMAL (if no matches)
 *   AVY_HINT --[hint char]--> AVY_ACTION (when unique)
 *   AVY_ACTION --[j]--> NORMAL (after jump)
 *   ANY --[Escape]--> NORMAL
 */
enum app_mode {
	MODE_NORMAL,	 /* Normal editing */
	MODE_AVY_CHAR,	 /* Waiting for search character */
	MODE_AVY_HINT,	 /* Waiting for hint selection */
	MODE_AVY_ACTION, /* Waiting for action key */
};

struct app_state {
	bool running;
	bool needs_redraw;
	struct ui_input input;
	struct buffer buffer;
	struct font_ctx *font;
	struct syntax_ctx *syntax;
	struct view view;
	struct syntax_visible visible_ast;
	enum app_mode mode;
	struct avy_state avy;
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
	struct str line;
	char *cstr;

	line = buffer_get_current_line(&app->buffer);
	cstr = str_to_cstr(line);
	ui_input_set_text(&app->input, cstr);
	free(cstr);
}

/* ============================================================
 * INPUT HANDLING
 * ============================================================ */

static bool handle_key_normal(struct app_state *app,
			      uint32_t keysym,
			      uint32_t mods,
			      uint32_t codepoint);
static bool handle_key_avy_char(struct app_state *app,
				uint32_t keysym,
				uint32_t mods,
				uint32_t codepoint);
static bool handle_key_avy_hint(struct app_state *app,
				uint32_t keysym,
				uint32_t mods,
				uint32_t codepoint);
static bool handle_key_avy_action(struct app_state *app,
				  uint32_t keysym,
				  uint32_t mods,
				  uint32_t codepoint);
static void execute_jump_action(struct app_state *app,
				struct avy_match *match);

static bool
handle_key(struct app_state *app,
	   uint32_t keysym,
	   uint32_t mods,
	   uint32_t codepoint)
{
	/* Escape always cancels avy mode from any state */
	if (keysym == XKB_KEY_Escape) {
		if (app->mode != MODE_NORMAL) {
			app->mode = MODE_NORMAL;
			avy_cancel(&app->avy);
			return true;
		}
		/* Fall through to existing escape handling */
	}

	switch (app->mode) {
	case MODE_NORMAL:
		return handle_key_normal(app, keysym, mods, codepoint);

	case MODE_AVY_CHAR:
		return handle_key_avy_char(app, keysym, mods, codepoint);

	case MODE_AVY_HINT:
		return handle_key_avy_hint(app, keysym, mods, codepoint);

	case MODE_AVY_ACTION:
		return handle_key_avy_action(app, keysym, mods, codepoint);
	}

	return false;
}
static bool
handle_key_normal(struct app_state *app,
		  uint32_t keysym,
		  uint32_t mods,
		  uint32_t codepoint)
{
	/* Alt-; starts avy search upward */
	if ((mods & MOD_ALT) && keysym == XKB_KEY_semicolon) {
		app->mode = MODE_AVY_CHAR;
		avy_start(&app->avy, AVY_DIR_UP);
		return true;
	}

	/* Alt-' starts avy search downward */
	if ((mods & MOD_ALT) && keysym == XKB_KEY_apostrophe) {
		app->mode = MODE_AVY_CHAR;
		avy_start(&app->avy, AVY_DIR_DOWN);
		return true;
	}

	/* Buffer navigation keys (Ctrl-N, Ctrl-P) */
	if (mods & MOD_CTRL) {
		switch (keysym) {
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

	/* Let input component try to handle the key */
	if (ui_input_handle_key(&app->input, keysym, mods, codepoint)) {
		return true;
	}

	/* Global keys */
	switch (keysym) {
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
		warn("Submitted: %s\n", ui_input_get_text(&app->input));
		return false;
	}

	return false;
}

static bool
handle_key_avy_char(struct app_state *app,
		    uint32_t keysym,
		    uint32_t mods,
		    uint32_t codepoint)
{
	(void)keysym;
	(void)mods;

	/* Wait for a printable ASCII character */
	if (codepoint >= 32 && codepoint < 127) {
		avy_set_char(&app->avy,
			     (char)codepoint,
			     app->buffer.lines,
			     app->buffer.line_count,
			     app->buffer.cursor_line,
			     app->view.first_visible_line,
			     app->view.last_visible_line);

		if (app->avy.match_count == 0) {
			/* No matches found, cancel */
			app->mode = MODE_NORMAL;
			avy_cancel(&app->avy);
		} else if (app->avy.match_count == 1) {
			/* Single match, skip hint selection, go to action */
			app->avy.selected_match = 0;
			app->mode = MODE_AVY_ACTION;
		} else {
			/* Multiple matches, wait for hint input */
			app->mode = MODE_AVY_HINT;
		}
		return true;
	}

	return false;
}

static bool
handle_key_avy_hint(struct app_state *app,
		    uint32_t keysym,
		    uint32_t mods,
		    uint32_t codepoint)
{
	(void)keysym;
	(void)mods;

	/* Only accept lowercase hint characters */
	if (codepoint >= 'a' && codepoint <= 'z') {
		if (avy_input_hint(&app->avy, (char)codepoint)) {
			/* Selection complete, switch to action mode */
			app->mode = MODE_AVY_ACTION;
		}
		/* else: need more input, stay in AVY_HINT mode */
		return true;
	}

	return false;
}

static bool
handle_key_avy_action(struct app_state *app,
		      uint32_t keysym,
		      uint32_t mods,
		      uint32_t codepoint)
{
	struct avy_match *match;

	(void)keysym;
	(void)mods;

	match = avy_get_selected(&app->avy);
	if (!match) {
		/* Shouldn't happen, but handle gracefully */
		app->mode = MODE_NORMAL;
		avy_cancel(&app->avy);
		return true;
	}

	/* j = jump action */
	if (codepoint == 'j') {
		execute_jump_action(app, match);
		app->mode = MODE_NORMAL;
		avy_cancel(&app->avy);
		return true;
	}

	return false;
}

/*
 * Execute the jump action: move target line to input box.
 *
 * This moves the buffer cursor to the target line, syncs the
 * input box content, and positions the cursor at the word start
 * where the match was found.
 */
static void
execute_jump_action(struct app_state *app, struct avy_match *match)
{
	struct str line;
	char *cstr;

	/* Move buffer cursor to target line */
	app->buffer.cursor_line = match->line;

	/* Sync input box with new current line */
	line = buffer_get_current_line(&app->buffer);
	cstr = str_to_cstr(line);
	ui_input_set_text(&app->input, cstr);
	free(cstr);

	/* Position input cursor at word start */
	app->input.cursor = match->col;
	if (app->input.cursor > app->input.len)
		app->input.cursor = app->input.len;
}

/* ============================================================
 * RENDERING
 * ============================================================ */

static void
render(struct app_state *app, struct framebuffer *fb)
{
	struct ui_ctx ctx;
	int line_h, menu_h;
	int input_y, input_h;
	int lines_above, lines_below;
	int y, i, line_num;
	struct str line;
	int padding_x = 8;

	/*
	 * Track Y positions of visible lines for hint overlay.
	 * 128 entries is more than enough for any reasonable screen.
	 */
	int line_y_positions[128];
	int visible_line_count = 0;
	int first_visible = 0;

	ui_ctx_init(&ctx, fb, app->font);
	ui_ctx_clear(&ctx);

	line_h = font_get_line_height(app->font);
	menu_h = MENU_ROWS * line_h;

	input_h = line_h + 4;
	input_y = (fb->height - input_h) / 2;

	if (view_update(&app->view,
			app->buffer.cursor_line,
			app->buffer.line_count,
			fb->height,
			line_h,
			menu_h)) {
		if (syntax_has_tree(app->syntax)) {
			struct str source = buffer_get_text(&app->buffer);
			syntax_get_visible_nodes(
			    app->syntax,
			    source,
			    (uint32_t)app->view.first_visible_line,
			    (uint32_t)app->view.last_visible_line,
			    &app->visible_ast);
		}
	}

	lines_above = input_y / line_h;
	lines_below = (fb->height - input_y - input_h - menu_h) / line_h;

	/* Calculate first visible line for coordinate mapping */
	first_visible = app->buffer.cursor_line - lines_above;
	if (first_visible < 0)
		first_visible = 0;

	/* Draw lines above cursor */
	for (i = 0; i < lines_above; i++) {
		line_num = app->buffer.cursor_line - (lines_above - i);
		if (line_num < 0)
			continue;

		y = i * line_h;

		/* Record Y position for this line (for hint overlay) */
		if (visible_line_count < 128) {
			line_y_positions[visible_line_count++] = y;
		}

		line = buffer_get_line(&app->buffer, line_num);
		ui_label_draw_colored(
		    &ctx, padding_x, y, line, ctx.theme.fg_secondary);
	}

	/* Draw input box */
	{
		int cursor_x;
		ui_rect input_bg = {0, input_y, fb->width, input_h};
		draw_rect(&ctx.render, input_bg, ctx.theme.bg_hover);

		int text_y = input_y + (input_h - line_h) / 2;
		ui_label_draw_colored(&ctx,
				      padding_x,
				      text_y,
				      str_from_cstr(app->input.buf),
				      ctx.theme.fg_primary);

		cursor_x = padding_x + font_char_index_to_x(ctx.render.font,
							    str_from_cstr(app->input.buf),
							    app->input.cursor);
		ui_rect cursor_rect = {cursor_x, text_y, 2, line_h};
		draw_rect(&ctx.render, cursor_rect, ctx.theme.accent);
	}

	/* Draw lines below cursor */
	for (i = 0; i < lines_below; i++) {
		line_num = app->buffer.cursor_line + 1 + i;
		if (line_num >= app->buffer.line_count)
			break;

		y = input_y + input_h + (i * line_h);

		/* Record Y position for this line */
		if (visible_line_count < 128) {
			line_y_positions[visible_line_count++] = y;
		}

		line = buffer_get_line(&app->buffer, line_num);
		ui_label_draw_colored(
		    &ctx, padding_x, y, line, ctx.theme.fg_secondary);
	}

	/* Draw hint overlays when in hint selection mode */
	if (app->mode == MODE_AVY_HINT) {
		avy_draw_hints(&ctx,
			       &app->avy,
			       line_y_positions,
			       visible_line_count,
			       first_visible,
			       app->buffer.cursor_line,
			       padding_x);
	}

	/* Draw menu area (switches based on mode) */
	{
		ui_rect menu_rect;
		menu_rect.x = 0;
		menu_rect.y = fb->height - menu_h;
		menu_rect.w = fb->width;
		menu_rect.h = menu_h;

		if (app->mode == MODE_AVY_ACTION) {
			/* Show action menu with target context */
			struct avy_match *match = avy_get_selected(&app->avy);
			if (match) {
				struct str target_line =
				    buffer_get_line(&app->buffer, match->line);
				menu_actions_draw(&ctx,
						  menu_rect,
						  match,
						  target_line,
						  &app->visible_ast);
			}
		} else {
			/* Show AST debug view (default) */
			menu_ast_draw(&ctx,
				      menu_rect,
				      &app->visible_ast,
				      app->buffer.cursor_line);
		}
	}
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

	/* Init avy */
	app.mode = MODE_NORMAL;
	avy_init(&app.avy);

	/* Print PID for easy killing */
	dbg("PID: %d\n", getpid());

	/* Require filename argument */
	if (argc < 2)
		die("Usage: %s <file>\n", argv[0]);
	filepath = argv[1];

	/* Initialize buffer and load file */
	buffer_init(&app.buffer);
	if (!buffer_load(&app.buffer, filepath))
		die("Failed to load: %s\n", filepath);

	view_init(&app.view);

	app.syntax = syntax_create();
	if (app.syntax) {
		struct str source = buffer_get_text(&app.buffer);
		syntax_parse(app.syntax, source);
	}

	/* Load font */
	app.font = font_create("assets/fonts/JetBrainsMono-Regular.ttf", 20);
	if (!app.font)
		die("Failed to load font\n");

	/* Initialize input with first line */
	ui_input_init(&app.input);
	{
		char *cstr = str_to_cstr(buffer_get_current_line(&app.buffer));
		ui_input_set_text(&app.input, cstr);
		free(cstr);
	}

	/* Basic initialization of app */
	app.running = true;
	app.needs_redraw = true;

	/* Create window */
	platform = platform_create("Input Demo", 800, 600);
	if (!platform)
		die("Failed to create platform\n");

	printf("=== Single-Line Input Demo ===\n");
	printf("Type text. Readline shortcuts work.\n");
	printf("Enter to submit, Escape to quit.\n\n");

	/* Main loop */
	while (app.running) {
		struct platform_event ev;

		if (app.needs_redraw) {
			struct framebuffer *fb =
			    platform_get_framebuffer(platform);
			if (fb) {
				render(&app, fb);
				platform_present(platform);
			}
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
				if (handle_key(&app,
					       ev.key.keysym,
					       ev.key.modifiers,
					       ev.key.codepoint))
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
	syntax_destroy(app.syntax);
	buffer_destroy(&app.buffer);

	dbg("Clean shutdown\n");
	return 0;
}
