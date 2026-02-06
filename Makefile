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
CFLAGS += -Iinclude

# Vendor flags: no sanitizers, but needs tree-sitter includes
VENDOR_CFLAGS = -std=c99 -Wall -O2
VENDOR_CFLAGS += -Ivendor/tree-sitter/lib/include -Ivendor/tree-sitter/lib/src

# Source files (wildcard per module)
SRCS  = src/main.c
SRCS += $(wildcard src/core/*.c)
SRCS += $(wildcard src/render/*.c)
SRCS += $(wildcard src/platform/*.c)
SRCS += $(wildcard src/platform/protocols/*.c)
SRCS += $(wildcard src/editor/*.c)
SRCS += $(wildcard src/ui/*.c)

# Vendor source files (compile without sanitizers)
VENDOR_SRCS = vendor/stb/stb_truetype.c
VENDOR_SRCS += vendor/tree-sitter/lib/src/lib.c \
               vendor/tree-sitter-markdown/src/parser.c \
               vendor/tree-sitter-markdown/src/scanner.c

# Object files (in build/ directory)
OBJS = $(SRCS:%.c=build/%.o)
VENDOR_OBJS = $(VENDOR_SRCS:%.c=build/%.o)

# Default target
all: build/$(BIN_NAME)

# Link
build/$(BIN_NAME): $(OBJS) $(VENDOR_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)



# Compile application sources (with sanitizers)
build/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile vendor sources (without sanitizers to avoid false positives)
build/vendor/%.o: vendor/%.c
	@mkdir -p $(dir $@)
	$(CC) $(VENDOR_CFLAGS) -c -o $@ $<

# Clean
clean:
	rm -rf build/

# Run
run: all
	./build/$(BIN_NAME)

.PHONY: all clean run
