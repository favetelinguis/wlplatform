/* include/platform/platform.h
 *
 * Window management and input abstraction.
 * Layer 2 - depends on core/, render/ (for framebuffer type).
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include <render/render_types.h>

/* Modifier key flags */
#define MOD_CTRL  (1 << 0)
#define MOD_ALT   (1 << 1)
#define MOD_SHIFT (1 << 2)
#define MOD_SUPER (1 << 3)

/* Event types */
enum platform_event_type {
	EVENT_NONE = 0,
	EVENT_QUIT,
	EVENT_KEY_PRESS,
	EVENT_KEY_RELEASE,
	EVENT_RESIZE,
	EVENT_FOCUS_IN,
	EVENT_FOCUS_OUT,
};

struct platform_key_event {
	uint32_t keysym;
	uint32_t codepoint;
	uint32_t modifiers;
	uint32_t keycode;
};

struct platform_resize_event {
	int width;
	int height;
};

struct platform_event {
	enum platform_event_type type;
	uint32_t timestamp;
	union {
		struct platform_key_event key;
		struct platform_resize_event resize;
	};
};

struct platform; /* Opaque */

struct platform *platform_create(const char *title, int width, int height);
void platform_destroy(struct platform *p);
struct framebuffer *platform_get_framebuffer(struct platform *p);
void platform_present(struct platform *p);
bool platform_wait_events(struct platform *p, int timeout_ms);
bool platform_next_event(struct platform *p, struct platform_event *ev);

#endif /* PLATFORM_H */
