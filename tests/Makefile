ROOT = ..
BUILD_DIR = $(ROOT)/build/tests

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
CFLAGS += -g -fsanitize=address,undefined,leak -fno-omit-frame-pointer

# Include paths (relative to root)
CFLAGS += -I$(ROOT)/include

# Test sources (in tests/)
TEST_SRCS = test_arena.c test_astr.c test_afile.c
TEST_BINS = $(TEST_SRCS:%.c=$(BUILD_DIR)/%)

# Core sources needed by tests (relative to root)
CORE_SRCS = \
	$(ROOT)/src/core/error.c \
	$(ROOT)/src/core/memory.c \
	$(ROOT)/src/core/str.c \
	$(ROOT)/src/core/arena.c \
	$(ROOT)/src/core/astr.c \
	$(ROOT)/src/core/afile.c

# Object files
CORE_OBJS = $(CORE_SRCS:$(ROOT)/%.c=$(ROOT)/build/%.o)
VENDOR_OBJS = $(VENDOR_SRCS:$(ROOT)/%.c=$(ROOT)/build/%.o)

# Default: build and run all tests
all: $(TEST_BINS)
	@echo "Running tests..."
	@for t in $(TEST_BINS); do echo "=== $$t ==="; $$t || exit 1; done
	@echo "All tests passed."

# Build test binaries
$(BUILD_DIR)/%: %.c $(CORE_OBJS) 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $< $(CORE_OBJS)

# Build core objects (delegate to root if needed, or build here)
$(ROOT)/build/src/core/%.o: $(ROOT)/src/core/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean test artifacts only
clean:
	rm -rf $(BUILD_DIR)

# Run specific test
run-%: $(BUILD_DIR)/test_%

.PHONY: all clean run-%
