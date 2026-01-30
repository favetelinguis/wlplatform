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

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "platform/platform.h"
#include "render/render_font.h"

/* ============================================================
 * APPLICATION STATE
 * ============================================================ */

#define NUM_ITEMS 5

struct app_state {
	bool running;
	int focused_item; /* Currently focused menu item (0 to NUM_ITEMS-1) */
	const char *items[NUM_ITEMS];
	bool item_activated[NUM_ITEMS];
	struct font_ctx *font;
};

/* ============================================================
 * DRAWING FUNCTIONS
 * ============================================================ */

static void
draw_rect(
    struct framebuffer *fb, int rx, int ry, int rw, int rh, uint32_t color)
{
	int x, y;
	int x0, y0, x1, y1;

	x0 = rx < 0 ? 0 : rx;
	y0 = ry < 0 ? 0 : ry;
	x1 = (rx + rw) > fb->width ? fb->width : (rx + rw);
	y1 = (ry + rh) > fb->height ? fb->height : (ry + rh);

	for (y = y0; y < y1; y++) {
		for (x = x0; x < x1; x++) {
			fb->pixels[y * fb->width + x] = color;
		}
	}
}

static void
draw_rect_outline(struct framebuffer *fb,
		  int rx,
		  int ry,
		  int rw,
		  int rh,
		  uint32_t color,
		  int thickness)
{
	/* Top */
	draw_rect(fb, rx, ry, rw, thickness, color);
	/* Bottom */
	draw_rect(fb, rx, ry + rh - thickness, rw, thickness, color);
	/* Left */
	draw_rect(fb, rx, ry, thickness, rh, color);
	/* Right */
	draw_rect(fb, rx + rw - thickness, ry, thickness, rh, color);
}

static void
draw_text(struct framebuffer *fb,
	  struct font_ctx *font,
	  int x,
	  int y,
	  const char *text,
	  uint32_t color)
{
	/* y is top-left, but font_draw_text expect baseline */
	int baseline_y = y + font_get_ascent(font);
	font_draw_text(font,
		       fb->pixels,
		       fb->width,
		       fb->height,
		       x,
		       baseline_y,
		       text,
		       color);
}

/* ============================================================
 * INPUT HANDLING
 * ============================================================ */

static void
handle_key(struct app_state *app, struct platform_event *ev)
{
	uint32_t key = ev->key.keysym;
	uint32_t mods = ev->key.modifiers;

	/* Navigation */
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
		/* g = go to top */
		app->focused_item = 0;
		break;

	case XKB_KEY_G:
		/* G = got to bottom */
		app->focused_item = NUM_ITEMS - 1;
		break;

	case XKB_KEY_Return:
	case XKB_KEY_space:
		/* Activate current item */
		app->item_activated[app->focused_item] =
		    !app->item_activated[app->focused_item];
		printf("Item %d toggled: %s\n",
		       app->focused_item,
		       app->item_activated[app->focused_item] ? "ON" : "OFF");
		break;

	case XKB_KEY_Escape:
		app->running = false;
		break;

	case XKB_KEY_q:
		if (mods & MOD_CTRL) {
			app->running = false;
		}
		break;
	}
}

/* ============================================================
 * RENDERING
 * ============================================================ */

static void
render(struct platform *p, struct app_state *app)
{
	struct framebuffer *fb;
	int i;
	int item_height = 40;
	int item_width = 400;
	int start_x = 50;
	int start_y = 60;
	int line_height;

	fb = platform_get_framebuffer(p);
	if (!fb) {
		return;
	}

	line_height = font_get_line_height(app->font);

	/* Clear to dark background */
	for (i = 0; i < fb->width * fb->height; i++) {
		fb->pixels[i] = 0xFF1E1E1E;
	}

	/* Draw title */
	draw_text(
	    fb, app->font, start_x, 20, "Keyboard-Driven Menu", 0xFFFFFFFF);
	draw_text(fb,
		  app->font,
		  start_x,
		  20 + line_height,
		  "j/k to navigate, Enter to select",
		  0xFF808080);

	/* Draw menu items */
	for (i = 0; i < NUM_ITEMS; i++) {
		int y = start_y + i * (item_height + 10);
		bool focused = (i == app->focused_item);
		bool activated = app->item_activated[i];
		uint32_t bg_color, text_color;

		/* Item background */
		bg_color = activated ? 0xFF2D5A2D : 0xFF2D2D2D;
		if (focused) {
			bg_color = activated ? 0xFF3D7A3D : 0xFF3D3D3D;
		}
		draw_rect(fb, start_x, y, item_width, item_height, bg_color);

		/* Focus idicator (left border) */
		if (focused) {
			draw_rect(fb, start_x, y, 4, item_height, 0xFF007ACC);
		}

		/* Focus ring */
		if (focused) {
			draw_rect_outline(fb,
					  start_x - 2,
					  y - 2,
					  item_width + 4,
					  item_height + 4,
					  0xFF007ACC,
					  2);
		}

		/* Item text */
		text_color = focused ? 0xFFFFFFFF : 0xFFCCCCCC;
		draw_text(fb,
			  app->font,
			  start_x + 20,
			  y + (item_height - line_height) / 2,
			  app->items[i],
			  text_color);

		/* Status indicator */
		if (activated) {
			draw_text(fb,
				  app->font,
				  start_x + item_width - 60,
				  y + (item_height - line_height) / 2,
				  "[ON]",
				  0xFF00FF00);
		}
	}

	/* Draw mode indicator at bottom */
	draw_rect(fb, 0, fb->height - 30, fb->width, 30, 0xFF252525);
	draw_text(fb, app->font, 10, fb->height - 25, "NORMAL", 0xFF007ACC);

	/* Draw help text */
	draw_text(
	    fb,
	    app->font,
	    start_x,
	    fb->height - 25,
	    "j/k: navigate | Enter: toggle | g/G: first/last | Esc: quit",
	    0xFF606060);

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

	/* Main loop */
	while (app.running) {
		struct platform_event ev;
		/* Poll for Wayland events (non-blocking) */
		if (!platform_poll_events(platform)) {
			break; /* Display connection lost */
		}

		/* Process Wayland events */
		while (platform_next_event(platform, &ev)) {
			switch (ev.type) {
			case EVENT_QUIT:
				app.running = false;
				break;

			case EVENT_KEY_PRESS:
				handle_key(&app, &ev);
				break;

			case EVENT_KEY_RELEASE:
				/* Key released */
				break;

			case EVENT_FOCUS_IN:
				printf("Window gained focus\n");
				break;

			case EVENT_FOCUS_OUT:
				printf("Window lost focus\n");
				break;
			case EVENT_RESIZE:
				printf("Resized to %dx%d\n",
				       ev.resize.width,
				       ev.resize.height);

			default:
				break;
			}
		}
		/* Render */
		render(platform, &app);

		/* Frame limiting (~60 FPS) */
		{
			struct timespec sleep_time;
			sleep_time.tv_sec = 0;
			sleep_time.tv_nsec = 16000000;
			nanosleep(&sleep_time, NULL);
		}
	}

	/* Cleanup */
	platform_destroy(platform);
	font_destroy(app.font);
	printf("Clean shutdown\n");

	return 0;
}
