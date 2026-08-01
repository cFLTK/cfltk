# cfltk - plain Makefile alternative to CMakeLists.txt.
# Builds the core library + the X11 reference backend + examples.

CC      ?= cc
# -Wno-missing-field-initializers: Fl_Menu_Item arrays are meant to be
# written the same terse, positional way FLTK C++ apps write them
# (`{"&Open", 0, cb}`, trailing fields default to zero); -Wextra
# otherwise flags every single one of them.
CFLAGS  ?= -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -g -Iinclude
X11_CFLAGS := $(shell pkg-config --cflags x11 xft)
X11_LIBS   := $(shell pkg-config --libs x11 xft) -lm

CORE_SRCS := \
    src/core/Fl_Widget.c \
    src/core/Fl_Group.c \
    src/core/Fl.c \
    src/widgets/Fl_Window.c \
    src/widgets/Fl_Box.c \
    src/widgets/Fl_Button.c \
    src/widgets/Fl_Toggle_Button.c \
    src/widgets/Fl_Radio_Button.c \
    src/widgets/Fl_Light_Button.c \
    src/widgets/Fl_Round_Button.c \
    src/widgets/Fl_Check_Button.c \
    src/widgets/Fl_Return_Button.c \
    src/widgets/Fl_Repeat_Button.c \
    src/widgets/Fl_Input.c \
    src/widgets/Fl_Float_Input.c \
    src/widgets/Fl_Int_Input.c \
    src/widgets/Fl_Multiline_Input.c \
    src/widgets/Fl_Secret_Input.c \
    src/widgets/Fl_Output.c \
    src/widgets/Fl_Multiline_Output.c \
    src/menu/Fl_Menu_Item.c \
    src/menu/Fl_Menu_.c \
    src/menu/fl_menu_popup.c \
    src/menu/Fl_Menu_Button.c \
    src/menu/Fl_Choice.c \
    src/menu/Fl_Menu_Bar.c \
    src/draw/fl_draw.c \
    src/draw/fl_colormap.c \
    src/valuators/Fl_Valuator.c \
    src/valuators/Fl_Slider.c \
    src/valuators/Fl_Fill_Slider.c \
    src/valuators/Fl_Hor_Slider.c \
    src/valuators/Fl_Hor_Fill_Slider.c \
    src/valuators/Fl_Nice_Slider.c \
    src/valuators/Fl_Hor_Nice_Slider.c \
    src/valuators/Fl_Value_Slider.c \
    src/valuators/Fl_Hor_Value_Slider.c \
    src/valuators/Fl_Scrollbar.c \
    src/valuators/Fl_Dial.c \
    src/valuators/Fl_Fill_Dial.c \
    src/valuators/Fl_Line_Dial.c \
    src/valuators/Fl_Counter.c \
    src/valuators/Fl_Simple_Counter.c \
    src/valuators/Fl_Roller.c

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
	$(CC) $(CFLAGS) examples/buttons/buttons.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/buttons
	$(CC) $(CFLAGS) examples/radio/radio.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/radio
	$(CC) $(CFLAGS) examples/input/input.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/input
	$(CC) $(CFLAGS) examples/menus/menus.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/menus
	$(CC) $(CFLAGS) examples/valuators/valuators.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/valuators

clean:
	rm -f $(OBJS) $(LIB)
	rm -rf $(BUILD_DIR)
