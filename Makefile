BIN_NAME = app

# Compiler settings
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
CFLAGS += -g -fsanitize=address,undefined,leak -fno-omit-frame-pointer

CFLAGS += $(shell pkg-config --cflags wayland-client xkbcommon)
LIBS = $(shell pkg-config --libs wayland-client xkbcommon)
LIBS += -lrt -lm

# Include vendor libraries
CFLAGS += -Ivendor/stb
CFLAGS += -Ivendor/tree-sitter/lib/include -Ivendor/tree-sitter/lib/src

# Vendor flags: no sanitizers, but needs tree-sitter includes
VENDOR_CFLAGS = -std=c99 -Wall -O2
VENDOR_CFLAGS += -Ivendor/tree-sitter/lib/include -Ivendor/tree-sitter/lib/src

# Source files
SRCS = src/main.c \
       src/platform/platform_wayland.c \
       src/protocols/xdg-shell.c \
       src/render/render_font.c \
       src/render/render_primitives.c \
       src/string/str.c \
       src/string/str_buf.c \
       src/string/arena.c \
       src/buffer/buffer.c \
       src/ui/ui_ctx.c \
       src/ui/ui_label.c \
       src/ui/ui_button.c \
       src/ui/ui_input.c \
       src/ui/ui_panel.c

# Vendor source files (compile without sanitizers)
VENDOR_SRCS = vendor/stb/stb_truetype.c
VENDOR_SRCS += vendor/tree-sitter/lib/src/lib.c \
               vendor/tree-sitter-markdown/src/parser.c \
               vendor/tree-sitter-markdown/src/scanner.c

# Object files (in bin/ directory)
OBJS = $(SRCS:%.c=bin/%.o)
VENDOR_OBJS = $(VENDOR_SRCS:%.c=bin/%.o)

# Default target
all: bin/$(BIN_NAME)

# Link
bin/$(BIN_NAME): $(OBJS) $(VENDOR_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)



# Compile application sources (with sanitizers)
bin/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile vendor sources (without sanitizers to avoid false positives)
bin/vendor/%.o: vendor/%.c
	@mkdir -p $(dir $@)
	$(CC) $(VENDOR_CFLAGS) -c -o $@ $<

# Clean
clean:
	rm -rf bin/

# Run
run: all
	./bin/$(BIN_NAME)

.PHONY: all clean run
