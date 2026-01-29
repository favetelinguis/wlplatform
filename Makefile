BIN_NAME = app

# Compiler settings
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
CFLAGS += -g -fsanitize=address,undefined,leak -fno-omit-frame-pointer
CFLAGS += $(shell pkg-config --cflags wayland-client xkbcommon)

LIBS = $(shell pkg-config --libs wayland-client xkbcommon)
LIBS += -lrt  # For shm_open

# Source files
SRCS = src/main.c \
       src/platform/platform_wayland.c \
       src/xdg-shell.c

# Object files (in bin/ directory)
OBJS = $(SRCS:%.c=bin/%.o)

# Default target
all: bin/$(BIN_NAME)

# Link
bin/$(BIN_NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile
bin/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Header dependencies
bin/main.o: platform/platform.h
bin/platform/platform_wayland.o: platform/platform.h xdg-shell.h

# Clean
clean:
	rm -rf bin/

# Run
run: all
	./bin/$(BIN_NAME)

.PHONY: all clean run
