/* platform/platform.h
 *
 * Platform abstraction layer, provides window management, KEYBOARD input, and
 * framebuffer access.
 *
 * DESIGN: This is a keyboard-onpy platform layer. No mouse support.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Opaque platform context.
 * All platform state is hidden behind this pointer.
 */
struct platform;

/*
 * Event types that the platform can generate.
 */
enum event_type {
  EVENT_NONE = 0,
  EVENT_QUIT,        /* Window close requested */
  EVENT_RESIZE,      /* Window size changed */
  EVENT_KEY_PRESS,   /* Key pressed */
  EVENT_KEY_RELEASE, /* Key released */
  EVENT_FOCUS_IN,    /* Window gained keyboard focus */
  EVENT_FOCUS_OUT,   /* Window lost keyboard focus */
};

/*
 * Modifier key flags.
 */
#define MOD_SHIFT (1 << 0)
#define MOD_CTRL (1 << 1)
#define MOD_ALT (1 << 2)
#define MOD_SUPER (1 << 3)

/*
 * Platfor event structure.
 */
struct platform_event {
  enum event_type type;
  uint32_t timestamp; /* Milliseconds for timing*/
  union {
    /* EVENT_RESIZE */
    struct {
      int width, height;
    } resize;
    /* EVENT_KEY_PRESS, EVENT_KEY_RELEASE */
    struct {
      uint32_t keycode;   /* Raw keycode */
      uint32_t keysym;    /* XKB keysym (e.g., XKB_KEY_a) */
      uint32_t modifiers; /* MOD_SHIFT | MOD_CTRL | ... */
      uint32_t codepoint; /* UTF-32 character, or 0 if not printable */
    } key;
  };
};

/*
 * Framebuffer for rendering.
 * You draw into pixels[], then call platform_present().
 */
struct framebuffer {
  uint32_t *pixels; /* XRGB8888 format (0xAARRGGBB, AA ignored) */
  int width;
  int height;
  int stride; /* Bytes per row (usually width * 4) */
};

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

/*
 * Create platform context and open a window.
 *
 * @param title  Window title
 * @param width  Proposed initial width, actual may change
 * @param height Proposed initial height, actual may change
 * @return       Platform context, or NULL on failure
 */
struct platform *platform_create(const char *title, int width, int height);

/*
 * Destroy platform context and close window.
 * Safe to call with NULL.
 */
void platform_destroy(struct platform *p);

/* ============================================================
 * EVENT HANDLING
 * ============================================================ */

/*
 * Poll for events (non-blocking).
 *
 * @return false if the display connection was lost
 */
bool platform_poll_events(struct platform *p);

/*
 * Get the next event from the queue.
 *
 * @param out  Event structure to fill
 * @return     true if an event was available, false if queue empty
 */
bool platform_next_event(struct platform *p, struct platform_event *out);

/*
 * Check if window close was requested.
 */
bool platform_should_close(struct platform *p);

/* ============================================================
 * WINDOW INFO
 * ============================================================ */

/*
 * Get current window dimensions.
 */
int platform_get_width(struct platform *p);
int platform_get_height(struct platform *p);

/*
 * Check if window has keyboard focus.
 */
bool platform_has_focus(struct platform *p);

/* ============================================================
 * RENDERING
 * ============================================================ */

/*
 * Get the framebuffer for rendering.
 *
 * The returned framebuffer is valid until the next call to
 * platform_present() or platform_destroy().
 *
 * @return Framebuffer to draw into, or NULL if not ready
 */
struct framebuffer *platform_get_framebuffer(struct platform *p);

/*
 * Present the framebuffer to the screen.
 */
void platform_present(struct platform *p);

/* ============================================================
 * INPUT STATE (for immediate mode GUI)
 * ============================================================ */

/*
 * Get current modifier key state.
 * Useful for checking if Ctrl/Shift/Alt is held.
 */
uint32_t platform_get_modifiers(struct platform *p);

#endif /* PLATFORM_H */
