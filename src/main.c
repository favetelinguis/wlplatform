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

/* ============================================================
 * APPLICATION STATE
 * ============================================================ */

#define NUM_ITEMS 5

struct app_state {
	bool running;
	int focused_item; /* Currently focused menu item (0 to NUM_ITEMS-1) */
	const char *items[NUM_ITEMS];
	bool item_activated[NUM_ITEMS];
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
draw_text_placeholder(
    struct framebuffer *fb, int x, int y, const char *text, uint32_t color)
{
	/* Placeholder: draw a colored rectangle representing text */
	/* Real text rendering will come in a later step */
	int len = 0;
	while (text[len])
		len++;
	draw_rect(fb, x, y, len * 10, 20, color);
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
	int item_width = 300;
	int start_x = 50;
	int start_y = 50;

	fb = platform_get_framebuffer(p);
	if (!fb) {
		return;
	}

	/* Clear to dark background */
	for (i = 0; i < fb->width * fb->height; i++) {
		fb->pixels[i] = 0xFF1E1E1E;
	}

	/* Braw title */
	draw_text_placeholder(
	    fb,
	    start_x,
	    20,
	    "Keyboard-Driven Menu (j/k to navigate, Enter to select)",
	    0xFF808080);

	/* Draw menu items */
	for (i = 0; i < NUM_ITEMS; i++) {
		int y = start_y + i * (item_height + 10);
		bool focused = (i == app->focused_item);
		bool activated = app->item_activated[i];

		/* Item background */
		uint32_t bg_color = activated ? 0xFF2D5A2D : 0xFF2D2D2D;
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

		/* Item text placeholder */
		draw_text_placeholder(fb,
				      start_x + 20,
				      y + 10,
				      app->items[i],
				      focused ? 0xFFFFFFFF : 0xFFCCCCCC);
	}

	/* Draw mode indicator at bottom */
	draw_text_placeholder(fb, 10, fb->height - 30, "NORMAL", 0xFF007ACC);

	/* Draw help text */
	draw_text_placeholder(
	    fb,
	    start_x,
	    fb->height - 60,
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
		return 1;
	}
	printf("=== Keyboard-Driven UI Demo ===\n");
	printf("Window size: %dx%d\n",
	       platform_get_width(platform),
	       platform_get_height(platform));
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
		struct timespec sleep_time = {
		    .tv_sec = 0, .tv_nsec = 16000000 /* 16ms = ~60 FPS */
		};
		nanosleep(&sleep_time, NULL);
	}

	/* Cleanup */
	platform_destroy(platform);
	printf("Clean shutdown\n");

	return 0;
}
