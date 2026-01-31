/* main.c
 *
 * Example keyboard-driven application using the platform abstraction layer.
 * Demonstrates keyboard navigation and visual focus indication.
 *
 * Controls:
 *   j/Down    - Move focus down
 *   k/Up      - Move focus up
 *   Enter     - Activate focused item
 *   Escape    - Quit
 *   Ctrl+Q    - Quit
 */

#include <stdint.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "platform/platform.h"
#include "render/render_font.h"
#include "ui/ui.h"
#include "ui/ui_button.h"
#include "ui/ui_label.h"

/* ============================================================
 * APPLICATION STATE
 * ============================================================ */

#define NUM_ITEMS 5

struct app_state {
	bool running;
	bool needs_redraw; /* Dirty flag - only render when true */
	int focused_item;  /* Currently focused menu item (0 to NUM_ITEMS-1) */
	const char *items[NUM_ITEMS];
	bool item_activated[NUM_ITEMS];
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
	int old_focus = app->focused_item;

	switch (key) {
	case XKB_KEY_j:
	case XKB_KEY_Down:
		app->focused_item++;
		if (app->focused_item >= NUM_ITEMS) {
			app->focused_item = NUM_ITEMS - 1;
		}
		break;

	case XKB_KEY_k:
	case XKB_KEY_Up:
		app->focused_item--;
		if (app->focused_item < 0) {
			app->focused_item = 0;
		}
		break;

	case XKB_KEY_g:
		app->focused_item = 0;
		break;

	case XKB_KEY_G:
		app->focused_item = NUM_ITEMS - 1;
		break;

	case XKB_KEY_Return:
	case XKB_KEY_space:
		app->item_activated[app->focused_item] =
		    !app->item_activated[app->focused_item];
		printf("Item %d toggled: %s\n",
		       app->focused_item,
		       app->item_activated[app->focused_item] ? "ON" : "OFF");
		return true; /* State changed */

	case XKB_KEY_Escape:
		app->running = false;
		return false;

	case XKB_KEY_q:
		if (mods & MOD_CTRL) {
			app->running = false;
		}
		return false;

	default:
		return false;
	}

	return app->focused_item != old_focus; /* True if focus changed */
}

/* ============================================================
 * RENDERING
 * ============================================================ */

static void
render(struct platform *p, struct app_state *app)
{
	struct framebuffer *fb;
	struct ui_ctx ctx;
	int i;
	int line_height;

	fb = platform_get_framebuffer(p);
	if (!fb) {
		return;
	}

	ui_ctx_init(&ctx, fb, app->font);
	ui_ctx_clear(&ctx);

	line_height = font_get_line_height(app->font);

	/* Draw title */
	ui_label_draw(&ctx, 50, 20, "Keyboard-Driven Menu", UI_LABEL_NORMAL);
	ui_label_draw(&ctx,
		      50,
		      20 + line_height,
		      "j/k to navigate, Enter to select",
		      UI_LABEL_MUTED);

	/* Draw menu items */
	for (i = 0; i < NUM_ITEMS; i++) {
		struct ui_rect rect = {50, 60 + i * 50, 400, 40};
		struct ui_button_cfg cfg =
		    ui_button_cfg_default(app->items[i]);

		if (i == app->focused_item) {
			cfg.state |= UI_BTN_FOCUSED;
		}
		if (app->item_activated[i]) {
			cfg.state |= UI_BTN_ACTIVE;
			cfg.status_text = "[ON]";
		}
		ui_button_draw(&ctx, rect, &cfg);
	}

	/* Status bar */
	struct ui_rect status_rect = {0, fb->height - 30, fb->width, 30};
	ui_panel_draw(&ctx,
		      status_rect,
		      0xFF2F2F2F,
		      UI_PANEL_FLAT); /* Darker than bg_primary */
	ui_label_draw(&ctx, 10, fb->height - 25, "NORMAL", UI_LABEL_ACCENT);
	ui_label_draw(
	    &ctx,
	    50,
	    fb->height - 25,
	    "j/k: navigate | Enter: toggle | g/G: first/last | Esc: quit",
	    UI_LABEL_MUTED);

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

	/* Initialize app state */
	app.running = true;
	app.focused_item = 0;
	app.items[0] = "Option 1: Enable Feature A";
	app.items[1] = "Option 2: Enable Feature B";
	app.items[2] = "Option 3: Show Debug Info";
	app.items[3] = "Option 4: Dark Mode";
	app.items[4] = "Option 5: Exit Application";

	/* Create platform and window */
	platform = platform_create("Keyboard-Driven UI Demo", 800, 600);
	if (!platform) {
		fprintf(stderr, "Failed to create platform\n");
		font_destroy(app.font);
		return 1;
	}
	printf("=== Keyboard-Driven UI Demo ===\n");
	printf("Window size: %dx%d\n",
	       platform_get_width(platform),
	       platform_get_height(platform));
	printf("Font: JetBrains Mono %dpx\n", font_get_size(app.font));
	printf("Line height: %dpx\n", font_get_line_height(app.font));
	printf("\nControls:\n");
	printf("  j/Down  - Move focus down\n");
	printf("  k/Up    - Move focus up\n");
	printf("  g       - Go to first item\n");
	printf("  G       - Go to last item\n");
	printf("  Enter   - Toggle item\n");
	printf("  Escape  - Quit\n");
	printf("  Ctrl+Q  - Quit\n\n");

	/* Initial render */
	app.needs_redraw = true;

	/* Main loop */
	while (app.running) {
		struct platform_event ev;

		/* Render if needed (before waiting) */
		if (app.needs_redraw) {
			render(platform, &app);
			app.needs_redraw = false;
		}

		/* Block until event arrives (0% CPU when idle) */
		if (!platform_wait_events(platform, -1)) {
			break; /* Display connection lost */
		}

		/* Process all queued events */
		while (platform_next_event(platform, &ev)) {
			switch (ev.type) {
			case EVENT_QUIT:
				app.running = false;
				break;

			case EVENT_KEY_PRESS:
				if (handle_key(&app, &ev)) {
					app.needs_redraw = true;
				}
				break;

			case EVENT_RESIZE:
				printf("Resized to %dx%d\n",
				       ev.resize.width,
				       ev.resize.height);
				app.needs_redraw = true;
				break;

			case EVENT_FOCUS_IN:
				printf("Window gained focus\n");
				break;

			case EVENT_FOCUS_OUT:
				printf("Window lost focus\n");
				break;

			default:
				break;
			}
		}
	}

	/* Cleanup */
	platform_destroy(platform);
	font_destroy(app.font);
	printf("Clean shutdown\n");

	return 0;
}
