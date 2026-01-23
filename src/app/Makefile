BIN_NAME=wlclient

# Use pkg-config to dynamically get flags
CFLAGS=$(shell pkgconf --cflags wayland-client xkbcommon) -g -fsanitize=address,undefined,leak -fno-omit-frame-pointer
LIBS=$(shell pkgconf --libs wayland-client xkbcommon)

all:
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/${BIN_NAME} wlclient.c xdg-shell.c $(LIBS)

