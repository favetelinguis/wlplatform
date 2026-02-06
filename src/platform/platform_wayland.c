#define _POSIX_C_SOURCE 200809L

#include <core/memory.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "protocols/xdg-shell.h"
#include <platform/platform.h>

/* ============================================================
 * CONSTANTS
 * ============================================================ */

#define EVENT_QUEUE_SIZE 64
#define BUFFER_COUNT	 2

/* ============================================================
 * INTERNAL STRUCTURES
 * ============================================================ */

/*
 * Double-buffered framebuffer entry.
 */
struct buffer_entry {
	struct wl_buffer *wl_buffer;
	uint32_t *pixels;
	int fd;
	size_t size;
	bool busy; /* Compositor is using this buffer */
};

/*
 * Event queue (ring buffer)
 */
struct event_queue {
	struct platform_event events[EVENT_QUEUE_SIZE];
	int head; /* Read position */
	int tail; /* Write position */
};

/*
 * Main platform context - all Wayland state lives here.
 */
struct platform {
	/* Wayland globals */
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct xdg_wm_base *xdg_wm_base;
	struct wl_seat *seat;

	/* Window objects */
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *toplevel;

	/* Keyboard input */
	struct wl_keyboard *keyboard;

	/* XKB keyboard state */
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	/* Double buffering */
	struct buffer_entry buffers[BUFFER_COUNT];
	int current_buffer;

	/* Exposed framebuffer */
	struct framebuffer fb;

	/* Window state */
	int width;
	int height;
	bool configured; /* Has compositor sent initial configure? */
	bool closed;	 /* Close requested? */
	bool has_focus;	 /* Does window have keyboard focus? */

	/* Input state */
	uint32_t modifiers;   /* Current modifier state */
	uint32_t last_serial; /* TODO For clipboard operations later */

	/* Event queue */
	struct event_queue events;
};

/* ============================================================
 * EVENT QUEUE OPERATIONS
 * ============================================================ */

static void
event_queue_init(struct event_queue *q)
{
	q->head = 0;
	q->tail = 0;
}

static bool
event_queue_push(struct event_queue *q, struct platform_event *e)
{
	int next_tail = (q->tail + 1) % EVENT_QUEUE_SIZE;
	if (next_tail == q->head) {
		/* Queue full, drop oldest event */
		q->head = (q->head + 1) % EVENT_QUEUE_SIZE;
	}
	q->events[q->tail] = *e;
	q->tail = next_tail;
	return true;
}

static bool
event_queue_pop(struct event_queue *q, struct platform_event *out)
{
	if (q->head == q->tail) {
		return false; /* Empty */
	}
	*out = q->events[q->head];
	q->head = (q->head + 1) % EVENT_QUEUE_SIZE;
	return true;
}

/* ============================================================
 * SHARED MEMORY HELPERS
 * ============================================================ */

static void
randname(char *buf)
{
	struct timespec ts;
	long r;
	int i;

	clock_gettime(CLOCK_REALTIME, &ts);
	r = ts.tv_nsec;
	for (i = 0; i < 6; ++i) {
		buf[i] = 'A' + (r & 15) + (r & 16) * 2;
		r >>= 5;
	}
}

static int
create_shm_file(void)
{
	int retries = 100;
	int fd;

	do {
		char name[] = "/wl_shm-XXXXXX";
		randname(name + sizeof(name) - 7);
		--retries;
		fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			shm_unlink(name);
			return fd;
		}
	} while (retries > 0 && errno == EEXIST);

	return -1;
}

static int
allocate_shm_file(size_t size)
{
	int fd;
	int ret;

	fd = create_shm_file();
	if (fd < 0) {
		return -1;
	}

	do {
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);

	if (ret < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

/* ============================================================
 * BUFFER MANAGEMENT
 * ============================================================ */

static void
buffer_release(void *data, struct wl_buffer *wl_buffer)
{
	// TODO this is not the same as before
	struct buffer_entry *buf = data;
	buf->busy = false;
}

static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_release,
};

static bool
create_buffer(struct platform *p,
	      struct buffer_entry *buf,
	      int width,
	      int height)
{
	int stride;
	size_t size;
	struct wl_shm_pool *pool;

	stride = width * 4;
	size = stride * height;

	buf->fd = allocate_shm_file(size);
	if (buf->fd < 0) {
		return false;
	}

	buf->pixels =
	    xmmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, buf->fd, 0);

	pool = wl_shm_create_pool(p->shm, buf->fd, size);
	buf->wl_buffer = wl_shm_pool_create_buffer(
	    pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);

	buf->size = size;
	buf->busy = false;

	wl_buffer_add_listener(buf->wl_buffer, &buffer_listener, buf);

	return true;
}

static void
destroy_buffer(struct buffer_entry *buf)
{
	if (buf->wl_buffer) {
		wl_buffer_destroy(buf->wl_buffer);
		buf->wl_buffer = NULL;
	}
	if (buf->pixels && buf->pixels != MAP_FAILED) {
		xmunmap(buf->pixels, buf->size);
		buf->pixels = NULL;
	}
	if (buf->fd >= 0) {
		close(buf->fd);
		buf->fd = -1;
	}
}

static struct buffer_entry *
get_free_buffer(struct platform *p)
{
	int i;
	struct buffer_entry *buf;

	/* Find a non-busy buffer */
	for (i = 0; i < BUFFER_COUNT; i++) {
		int idx = (p->current_buffer + i) % BUFFER_COUNT;
		buf = &p->buffers[idx];
		if (!buf->busy) {
			p->current_buffer = idx;
			return buf;
		}
	}
	/* All buffers busy - shouldn't happen with double buffering */
	fprintf(stderr, "Warning: all buffers busy\n");
	return &p->buffers[p->current_buffer];
}

/* ============================================================
 * XDG SHELL LISTENERS
 * ============================================================ */

static void
xdg_surface_configure(void *data,
		      struct xdg_surface *xdg_surface,
		      uint32_t serial)
{
	struct platform *p = data;
	xdg_surface_ack_configure(xdg_surface, serial);
	p->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void
xdg_toplevel_configure(void *data,
		       struct xdg_toplevel *toplevel,
		       int32_t width,
		       int32_t height,
		       struct wl_array *states)
{
	struct platform *p = data;
	struct platform_event ev = {0};
	int i;

	if (width <= 0 || height <= 0) {
		return; /* Compositor deferring t us */
	}

	if (width != p->width || height != p->height) {
		/* Size changed - recreate buffers */
		for (i = 0; i < BUFFER_COUNT; i++) {
			destroy_buffer(&p->buffers[i]);
		}

		p->width = width;
		p->height = height;

		for (i = 0; i < BUFFER_COUNT; i++) {
			create_buffer(p, &p->buffers[i], width, height);
		}

		/* Queue resize event */
		ev.type = EVENT_RESIZE;
		ev.resize.width = width;
		ev.resize.height = height;
		event_queue_push(&p->events, &ev);
	}
}

static void
xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
	struct platform *p = data;
	struct platform_event ev = {0};

	p->closed = true;

	ev.type = EVENT_QUIT;
	event_queue_push(&p->events, &ev);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

/* ============================================================
 * KEYBOARD LISTENERS
 * ============================================================ */

static void
keyboard_keymap(void *data,
		struct wl_keyboard *keyboard,
		uint32_t format,
		int32_t fd,
		uint32_t size)
{
	struct platform *p = data;
	char *map_shm;

	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		return;
	}

	map_shm = xmmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

	/* Clean up old state */
	if (p->xkb_keymap) {
		xkb_keymap_unref(p->xkb_keymap);
	}
	if (p->xkb_state) {
		xkb_state_unref(p->xkb_state);
	}

	p->xkb_keymap =
	    xkb_keymap_new_from_string(p->xkb_context,
				       map_shm,
				       XKB_KEYMAP_FORMAT_TEXT_V1,
				       XKB_KEYMAP_COMPILE_NO_FLAGS);

	xmunmap(map_shm, size);
	close(fd);

	if (p->xkb_keymap) {
		p->xkb_state = xkb_state_new(p->xkb_keymap);
	}
}

static void
keyboard_enter(void *data,
	       struct wl_keyboard *keyboard,
	       uint32_t serial,
	       struct wl_surface *surface,
	       struct wl_array *keys)
{
	struct platform *p = data;
	struct platform_event ev = {0};

	p->last_serial = serial;
	p->has_focus = true;

	ev.type = EVENT_FOCUS_IN;
	event_queue_push(&p->events, &ev);

	/* Note: 'keys' contains currently pressed keys - we could process them
	 * but for simplicity we just acknowledge focus */
}

static void
keyboard_leave(void *data,
	       struct wl_keyboard *keyboard,
	       uint32_t serial,
	       struct wl_surface *surface)
{
	struct platform *p = data;
	struct platform_event ev = {0};

	p->last_serial = serial;
	p->has_focus = false;

	ev.type = EVENT_FOCUS_OUT;
	event_queue_push(&p->events, &ev);
}

/*
 * Convert UTF-8 to UTF-32 codepoint.
 * Returns the codepoint, or 0 if invalid/control character.
 */
static uint32_t
utf8_to_codepoint(const char *utf8, int len)
{
	unsigned char c;

	if (len <= 0) {
		return 0;
	}

	c = (unsigned char)utf8[0];

	/* Reject control characters */
	if (c < 32) {
		return 0;
	}

	/* ASCII */
	if (c < 0x80) {
		return c;
	}

	/* 2-byte sequence */
	if ((c & 0xE0) == 0xC0 && len >= 2) {
		return ((c & 0x1F) << 6) | (utf8[1] & 0x3F);
	}

	/* 3-byte sequence */
	if ((c & 0xF0) == 0xE0 && len >= 3) {
		return ((c & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) |
		       (utf8[2] & 0x3F);
	}

	/* 4-byte sequence */
	if ((c & 0xF8) == 0xF0 && len >= 4) {
		return ((c & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) |
		       ((utf8[2] & 0x3F) << 6) | (utf8[3] & 0x3F);
	}

	return 0;
}

static void
keyboard_key(void *data,
	     struct wl_keyboard *keyboard,
	     uint32_t serial,
	     uint32_t time,
	     uint32_t key,
	     uint32_t state)
{
	struct platform *p = data;
	struct platform_event ev = {0};
	uint32_t keycode;
	char utf8[8];
	int utf8_len;

	p->last_serial = serial;
	keycode = key + 8; /* Linux evdev to XKB offset */

	ev.type = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? EVENT_KEY_PRESS
							   : EVENT_KEY_RELEASE;
	ev.timestamp = time;
	ev.key.keycode = key;
	ev.key.keysym = xkb_state_key_get_one_sym(p->xkb_state, keycode);
	ev.key.modifiers = p->modifiers;
	ev.key.codepoint = 0;

	/* Get UTF-32 codepoint for text input (only on press) */
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		utf8_len = xkb_state_key_get_utf8(
		    p->xkb_state, keycode, utf8, sizeof(utf8));
		if (utf8_len > 0) {
			ev.key.codepoint = utf8_to_codepoint(utf8, utf8_len);
		}
	}

	event_queue_push(&p->events, &ev);
}

static void
keyboard_modifiers(void *data,
		   struct wl_keyboard *keyboard,
		   uint32_t serial,
		   uint32_t mods_depressed,
		   uint32_t mods_latched,
		   uint32_t mods_locked,
		   uint32_t group)
{
	struct platform *p = data;

	p->last_serial = serial;

	xkb_state_update_mask(p->xkb_state,
			      mods_depressed,
			      mods_latched,
			      mods_locked,
			      0,
			      0,
			      group);

	/* Update our modifier flags */
	p->modifiers = 0;
	if (xkb_state_mod_name_is_active(
		p->xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE)) {
		p->modifiers |= MOD_SHIFT;
	}
	if (xkb_state_mod_name_is_active(
		p->xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE)) {
		p->modifiers |= MOD_CTRL;
	}
	if (xkb_state_mod_name_is_active(
		p->xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE)) {
		p->modifiers |= MOD_ALT;
	}
	if (xkb_state_mod_name_is_active(
		p->xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE)) {
		p->modifiers |= MOD_SUPER;
	}
}

static void
keyboard_repeat_info(void *data,
		     struct wl_keyboard *keyboard,
		     int32_t rate,
		     int32_t delay)
{
	/* Key repeat not implemented - left to application layer */
	(void)data;
	(void)keyboard;
	(void)rate;
	(void)delay;
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info,
};

/* ============================================================
 * SEAT LISTENER
 * Note: We only handle keyboard, NOT pointer
 * ============================================================ */

static void
seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities)
{
	struct platform *p = data;
	bool have_keyboard;

	have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

	/* Keyboard */
	if (have_keyboard && !p->keyboard) {
		p->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(p->keyboard, &keyboard_listener, p);
	} else if (!have_keyboard && p->keyboard) {
		wl_keyboard_release(p->keyboard);
		p->keyboard = NULL;
	}

	/* NOTE: We deliberately ignore WL_SEAT_CAPABILITY_POINTER
	 * This is a keyboard-only framework */
}

static void
seat_name(void *data, struct wl_seat *seat, const char *name)
{
	/* Informational only */
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

/* ============================================================
 * REGISTRY LISTENER
 * ============================================================ */

static void
registry_global(void *data,
		struct wl_registry *registry,
		uint32_t name,
		const char *interface,
		uint32_t version)
{
	struct platform *p = data;

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		p->shm =
		    wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		p->compositor = wl_registry_bind(
		    registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		p->xdg_wm_base = wl_registry_bind(
		    registry, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(
		    p->xdg_wm_base, &xdg_wm_base_listener, p);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		p->seat =
		    wl_registry_bind(registry, name, &wl_seat_interface, 7);
		wl_seat_add_listener(p->seat, &seat_listener, p);
	}
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	/* Handle device removel - not critical for basic operation */
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

/* ============================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================ */

struct platform *
platform_create(const char *title, int width, int height)
{
	struct platform *p;
	int i;

	p = xcalloc(1, sizeof(*p));

	/* Initialize buffer fds to invalid */
	for (i = 0; i < BUFFER_COUNT; i++) {
		p->buffers[i].fd = -1;
	}

	event_queue_init(&p->events);

	/* Connect to Wayland display */
	p->display = wl_display_connect(NULL);
	if (!p->display) {
		fprintf(stderr, "Failed to connect to Wayland display\n");
		xfree(p);
		return NULL;
	}

	/* Initialize XKB context */
	p->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!p->xkb_context) {
		fprintf(stderr, "Failed to create XKB context\n");
		wl_display_disconnect(p->display);
		xfree(p);
		return NULL;
	}

	/* Get registry and discover interfaces */
	p->registry = wl_display_get_registry(p->display);
	wl_registry_add_listener(p->registry, &registry_listener, p);
	wl_display_roundtrip(p->display);

	/* Check we got required interfaces */
	if (!p->compositor || !p->shm || !p->xdg_wm_base) {
		fprintf(stderr, "Missing required Wayland interfaces\n");
		platform_destroy(p);
		return NULL;
	}

	/* Set initial size */
	p->width = width;
	p->height = height;

	/* Create initial buffers */
	for (i = 0; i < BUFFER_COUNT; i++) {
		if (!create_buffer(p, &p->buffers[i], width, height)) {
			fprintf(stderr, "Filed to create buffer %d\n", i);
			platform_destroy(p);
			return NULL;
		}
	}

	/* Create surface */
	p->surface = wl_compositor_create_surface(p->compositor);
	p->xdg_surface =
	    xdg_wm_base_get_xdg_surface(p->xdg_wm_base, p->surface);
	xdg_surface_add_listener(p->xdg_surface, &xdg_surface_listener, p);

	p->toplevel = xdg_surface_get_toplevel(p->xdg_surface);
	xdg_toplevel_add_listener(p->toplevel, &xdg_toplevel_listener, p);
	xdg_toplevel_set_title(p->toplevel, title);

	/* Commit surface to trigger configure */
	wl_surface_commit(p->surface);

	/* Wait for initial configure */
	while (!p->configured && wl_display_dispatch(p->display) != -1) {
		/* Wait */
	}

	return p;
}

void
platform_destroy(struct platform *p)
{
	int i;

	if (!p) {
		return;
	}

	/* Destroy buffers */
	for (i = 0; i < BUFFER_COUNT; i++) {
		destroy_buffer(&p->buffers[i]);
	}

	/* Destroy keyboard */
	if (p->keyboard) {
		wl_keyboard_release(p->keyboard);
	}

	/* Destroy XKB state */
	if (p->xkb_state) {
		xkb_state_unref(p->xkb_state);
	}
	if (p->xkb_keymap) {
		xkb_keymap_unref(p->xkb_keymap);
	}
	if (p->xkb_context) {
		xkb_context_unref(p->xkb_context);
	}

	/* Destroy window */
	if (p->toplevel) {
		xdg_toplevel_destroy(p->toplevel);
	}
	if (p->xdg_surface) {
		xdg_surface_destroy(p->xdg_surface);
	}
	if (p->surface) {
		wl_surface_destroy(p->surface);
	}

	/* Roundtrip to ensure destruction is processed */
	if (p->display) {
		wl_display_roundtrip(p->display);
	}

	/* Destroy globals */
	if (p->seat) {
		wl_seat_release(p->seat);
	}
	if (p->xdg_wm_base) {
		xdg_wm_base_destroy(p->xdg_wm_base);
	}
	if (p->shm) {
		wl_shm_destroy(p->shm);
	}
	if (p->compositor) {
		wl_compositor_destroy(p->compositor);
	}
	if (p->registry) {
		wl_registry_destroy(p->registry);
	}

	/* Disconnect display */
	if (p->display) {
		wl_display_disconnect(p->display);
	}

	xfree(p);
}

bool
platform_wait_events(struct platform *p, int timeout_ms)
{
	struct pollfd fds[1];
	int ret;

	/* Prepare to read events */
	while (wl_display_prepare_read(p->display) != 0) {
		wl_display_dispatch_pending(p->display);
	}

	/* Flush outgoing requests */
	if (wl_display_flush(p->display) < 0) {
		wl_display_cancel_read(p->display);
		return false;
	}

	/* Blocking poll for events */
	fds[0].fd = wl_display_get_fd(p->display);
	fds[0].events = POLLIN;

	ret = poll(fds, 1, timeout_ms); /* BLOCKS here */

	if (ret > 0 && (fds[0].revents & POLLIN)) {
		if (wl_display_read_events(p->display) < 0) {
			return false;
		}
		wl_display_dispatch_pending(p->display);
	} else {
		wl_display_cancel_read(p->display);
	}

	return !p->closed;
}

bool
platform_next_event(struct platform *p, struct platform_event *out)
{
	return event_queue_pop(&p->events, out);
}

bool
platform_should_close(struct platform *p)
{
	return p->closed;
}

int
platform_get_width(struct platform *p)
{
	return p->width;
}

int
platform_get_height(struct platform *p)
{
	return p->height;
}

bool
platform_has_focus(struct platform *p)
{
	return p->has_focus;
}

struct framebuffer *
platform_get_framebuffer(struct platform *p)
{
	struct buffer_entry *buf;

	buf = get_free_buffer(p);

	p->fb.pixels = buf->pixels;
	p->fb.width = p->width;
	p->fb.height = p->height;
	p->fb.stride = p->width * 4;

	return &p->fb;
}

void
platform_present(struct platform *p)
{
	struct buffer_entry *buf;

	buf = &p->buffers[p->current_buffer];
	buf->busy = true;

	wl_surface_attach(p->surface, buf->wl_buffer, 0, 0);
	wl_surface_damage_buffer(p->surface, 0, 0, p->width, p->height);
	wl_surface_commit(p->surface);
	wl_display_flush(p->display);

	/* Move to next buffer for next frame */
	p->current_buffer = (p->current_buffer + 1) % BUFFER_COUNT;
}

uint32_t
platform_get_modifiers(struct platform *p)
{
	return p->modifiers;
}
