# Install
BIN = demo

# Flags
CFLAGS = -std=c99 -pedantic -O2 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -Iext

SRC = main.c  SDL_ime.c SDL_dbus.c SDL_ibus.c SDL_fcitx.c
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLU32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LIBS = -lglfw3 -framework OpenGL -lm
	else
		LIBS = -lglfw -lGL -lm -lGLU -ldbus-1 -ldl
	endif
endif

$(BIN):
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)
