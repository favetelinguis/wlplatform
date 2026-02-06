# C Vibe

Wayland text editor written in C99 with arena memory management, tree-sitter syntax highlighting, and avy-style jump navigation.

## Build

```
make clean && make       # build with sanitizers (asan, ubsan, leak)
make run                 # build and run
make -C tests            # run unit tests (test_arena, test_astr, test_afile)
```

Compiler: gcc, C99, `-Wall -Wextra -Wpedantic` with `-fsanitize=address,undefined,leak`.
Vendor code is compiled separately without sanitizers (`VENDOR_CFLAGS`).

## Project Structure

Headers live in `include/<module>/`, sources in `src/<module>/`.
All includes use angle brackets via `-Iinclude`: `#include <module/header.h>`.

```
include/
  core/       Layer 0    str.h arena.h astr.h memory.h error.h afile.h
  render/     Layer 1    render_types.h render_font.h render_primitives.h
  platform/   Layer 2    platform.h
  editor/     Layer 3    buffer.h view.h syntax.h
  ui/         Layer 4    ui.h ui_types.h ui_ctx.h ui_label.h ui_input.h
                         ui_button.h ui_panel.h ui_avy.h ui_menu_ast.h
                         ui_menu_actions.h

src/
  core/       str.c arena.c astr.c memory.c error.c afile.c
  render/     render_font.c render_primitives.c
  platform/   platform_wayland.c  protocols/xdg-shell.{c,h}
  editor/     buffer.c view.c syntax.c
  ui/         ui_ctx.c ui_label.c ui_input.c ui_button.c ui_panel.c
              ui_avy.c ui_menu_ast.c ui_menu_actions.c
  main.c
```

## Architecture: Layered Dependencies

Strict layered model. No upward or circular dependencies allowed.

```
Layer 0  core/      -> (nothing)
Layer 1  render/    -> core/
Layer 2  platform/  -> core/, render/
Layer 3  editor/    -> core/
Layer 4  ui/        -> core/, render/, platform/, editor/
         main.c     -> everything
```

**When adding new code, respect these rules:**
- A header in layer N must never include a header from layer N+1 or above.
- `render/` must not include `platform/`, `editor/`, or `ui/`.
- `editor/` must not include `render/`, `platform/`, or `ui/`.
- `platform/` must not include `editor/` or `ui/`.

## Key Design Patterns

### Render context threading
`struct render_ctx` (framebuffer + font) is embedded in `struct ui_ctx` (render + theme).
Render primitives take `struct render_ctx *`. UI functions take `struct ui_ctx *` and pass `&ctx->render` down to render calls.

### Rect types
`struct render_rect` is defined in `render_types.h`. UI code uses `typedef struct render_rect ui_rect;` from `ui_types.h`.

### Framebuffer ownership
`struct framebuffer` is defined in `render_types.h` (Layer 1). Platform creates and manages the actual pixel buffer but the type lives at the render layer so render primitives can use it without depending on platform.

### Avy decoupling
`ui_avy` does not depend on `editor/buffer`. The `avy_set_char()` function takes `const struct str *lines, int line_count` instead of a buffer pointer. The caller (main.c) passes `buffer->lines` and `buffer->line_count`.

### Memory management
- Arena allocator (`core/arena.h`) is the primary allocator for buffer content and line arrays.
- `core/memory.h` wraps malloc/free with `xmalloc`/`xcalloc`/`xfree` (abort on failure).
- `core/str.h` is a non-owning string view (`const char *data` + `int len`).
- `core/astr.h` provides arena-allocated string operations.

## Known Quirks

- `platform.h` uses an anonymous union in `struct platform_event` (C11 extension accepted by gcc in C99 mode, triggers `-Wpedantic` warning).
- Wayland callback functions have many unused parameters (inherent to the Wayland API, triggers `-Wunused-parameter` warnings).
- `stb_truetype.h` include in `render_font.c` uses a relative path to vendor/.

## Dependencies

- **System:** wayland-client, xkbcommon (via pkg-config)
- **Vendor:** stb_truetype (font rendering), tree-sitter + tree-sitter-markdown (syntax parsing)
