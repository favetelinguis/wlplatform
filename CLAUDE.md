# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make        # Build the application
make run    # Build and run
make clean  # Remove build artifacts
```

Output binary: `bin/app`

## Architecture

This is an ASCII-only, keyboard-driven Wayland application written in C99 with software rendering.

### Module Structure

```
src/
├── main.c                      # Application entry, event loop, UI layout
├── buffer/
│   └── buffer.[ch]             # Text buffer (line array, cursor, file I/O)
├── platform/
│   ├── platform.h              # Platform abstraction API
│   └── platform_wayland.c      # Wayland implementation (double-buffered)
├── protocols/
│   └── xdg-shell.[ch]          # Auto-generated Wayland protocol bindings
├── render/
│   ├── render_font.[ch]        # stb_truetype-based text rendering
│   └── render_primitives.[ch]  # Rectangle and text drawing primitives
└── ui/
    ├── ui.h                    # Unified include for all UI components
    ├── ui_types.h              # Common types (ui_rect, ui_theme, ui_ctx)
    ├── ui_ctx.[ch]             # Rendering context, Zenburn theme
    ├── ui_label.[ch]           # Text labels (normal, muted, accent)
    ├── ui_input.[ch]           # Single-line text input with readline bindings
    ├── ui_button.[ch]          # Keyboard-focusable buttons (legacy)
    └── ui_panel.[ch]           # Panel/container backgrounds (legacy)
```

### Key Design Decisions

- **Keyboard-only**: No mouse/pointer support by design
- **Wayland-native**: Uses xdg-shell protocol, won't work on X11
- **Immediate-mode rendering**: Direct pixel manipulation into XRGB8888 framebuffer
- **Double buffering**: Two buffers managed by Wayland compositor
- **ASCII text only**: Font renderer supports characters 32-126, glyph atlas 512x512
- **Single-focus model**: One text input always focused, no focus cycling needed

### UI Layout

The application has a static, three-region layout:

```
┌─────────────────────────────────────────────┐
│  Buffer context (lines ABOVE cursor)        │  ← Secondary color
│  ...                                        │
│  previous line                              │
├─────────────────────────────────────────────┤
│  INPUT BOX (current line, always focused)   │  ← Primary color, highlighted bg
│  ▌cursor                                    │
├─────────────────────────────────────────────┤
│  next line                                  │  ← Secondary color
│  ...                                        │
│  Buffer context (lines BELOW cursor)        │
├─────────────────────────────────────────────┤
│                                             │
│  MENU AREA (15 lines high)                  │  ← Varies based on input state
│  Context-sensitive commands/options         │
│                                             │
└─────────────────────────────────────────────┘
```

**Layout details:**

- **Input box**: Vertically centered, full window width, contains current buffer line with cursor. Always focused (no focus cycling).
- **Buffer context**: Lines above and below input show surrounding file content. Number of visible lines adapts to window height.
- **Menu area**: Fixed 15-line height (`MENU_ROWS`) at bottom. Content varies depending on input state (command mode, search mode, etc.).

**Rendering** (`main.c:render()`):
1. Calculate input box position (vertically centered)
2. Calculate available lines above/below based on remaining space
3. Draw buffer lines above (previous lines, secondary color)
4. Draw input box (highlighted background, primary text, cursor)
5. Draw buffer lines below (next lines, secondary color)
6. Draw menu panel at bottom

### Data Flow

1. Wayland keyboard events → event queue (ring buffer, 64 max)
2. Main loop blocks on `platform_wait_events()` until input arrives (0% CPU when idle)
3. Process events, update app state, set `needs_redraw` flag
4. Render to framebuffer via `platform_get_framebuffer()` using UI components
5. Present via `platform_present()` → `wl_surface_commit()`

### UI Component Pattern

UI components follow an immediate-mode pattern with `ui_ctx` passed to all draw functions:

```c
struct ui_ctx ctx;
ui_ctx_init(&ctx, fb, font);
ui_ctx_clear(&ctx);

/* Draw text input (text + cursor) */
ui_label_draw_colored(&ctx, x, y, app->input.buf, ctx.theme.fg_primary);
int cursor_x = x + font_char_index_to_x(ctx.font, app->input.buf, app->input.cursor);
draw_rect(&ctx, (struct ui_rect){cursor_x, y, 2, line_height}, ctx.theme.accent);
```

Components use a Zenburn color theme defined in `ui_types.h`.

### Text Input Component (`ui_input`)

The input component displays the current buffer line and handles text editing. It is always focused and supports readline/emacs keybindings:

| Binding | Action |
|---------|--------|
| `Ctrl-A` | Beginning of line |
| `Ctrl-E` | End of line |
| `Ctrl-F` / `Right` | Forward char |
| `Ctrl-B` / `Left` | Backward char |
| `Alt-F` | Forward word |
| `Alt-B` | Backward word |
| `Ctrl-D` / `Delete` | Delete char |
| `Backspace` / `Ctrl-H` | Delete backward |
| `Ctrl-K` | Kill to end of line |
| `Ctrl-U` | Kill to beginning |
| `Ctrl-W` | Kill word backward |
| `Alt-D` | Kill word forward |

**Buffer navigation** (handled before input):

| Binding | Action |
|---------|--------|
| `Ctrl-N` | Move to next line |
| `Ctrl-P` | Move to previous line |

**Key handling pattern**: Input component handles keys first via `ui_input_handle_key()`. Returns `true` if consumed, `false` to pass to global handlers (Escape, Ctrl-Q).

```c
/* In handle_key() */
if (ui_input_handle_key(&app->input, key, mods, codepoint)) {
    return true;  /* Input consumed the key */
}
/* Fall through to global keys */
```

**State structure** (caller-owned, no heap allocation):

```c
struct ui_input {
    char buf[256];      /* Fixed buffer, NUL-terminated */
    int len;            /* Current length */
    int cursor;         /* Cursor position (0 to len) */
    int scroll_offset;  /* Horizontal scroll for long text */
};
```

**Rendering** is inlined in `main.c`'s `render()` function (not a separate draw function) since layout is application-specific.

## Code Style

Follows suckless.org C99 style, enforced by `.clang-format`:
- Tabs for indentation (8 spaces width)
- 79 character line limit
- Return type on separate line for function definitions
- Pointer asterisk adjacent to variable name: `char *p`

## Dependencies

System (via pkg-config): `wayland-client`, `xkbcommon`

Vendored: `stb_truetype` in `vendor/stb/`

Install on Debian/Ubuntu:
```bash
sudo apt-get install libwayland-dev libxkbcommon-dev pkg-config build-essential
```

## Development Notes

- Address/undefined/leak sanitizers enabled by default for app code
- Vendor code compiled without sanitizers to avoid false positives
- Event-driven rendering: only redraws when `needs_redraw` is set (no continuous loop)
- Format code with: `clang-format -i src/**/*.c src/**/*.h`
