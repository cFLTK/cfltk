# cfltk - plain Makefile alternative to CMakeLists.txt.
# Builds the core library + the X11 reference backend + examples.

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wno-unused-parameter -g -Iinclude
X11_CFLAGS := $(shell pkg-config --cflags x11 xft)
X11_LIBS   := $(shell pkg-config --libs x11 xft)

CORE_SRCS := \
    src/core/Fl_Widget.c \
    src/core/Fl_Group.c \
    src/core/Fl.c \
    src/widgets/Fl_Window.c \
    src/widgets/Fl_Box.c \
    src/draw/fl_draw.c \
    src/draw/fl_colormap.c

X11_SRCS := \
    src/backend/x11/fl_x11_window.c \
    src/backend/x11/fl_x11_event.c \
    src/backend/x11/fl_x11_driver.c

SRCS := $(CORE_SRCS) $(X11_SRCS)
OBJS := $(SRCS:.c=.o)

BUILD_DIR := build
LIB := $(BUILD_DIR)/libcfltk.a

.PHONY: all clean examples

all: $(LIB) examples

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

%.o: %.c
	$(CC) $(CFLAGS) $(X11_CFLAGS) -c $< -o $@

$(LIB): $(BUILD_DIR) $(OBJS)
	ar rcs $(LIB) $(OBJS)

examples: $(LIB)
	$(CC) $(CFLAGS) examples/hello/hello.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/hello

clean:
	rm -f $(OBJS) $(LIB)
	rm -rf $(BUILD_DIR)
