# cfltk - plain Makefile alternative to CMakeLists.txt.
# Builds the core library + the X11 reference backend + examples.

CC      ?= cc
# -Wno-missing-field-initializers: Fl_Menu_Item arrays are meant to be
# written the same terse, positional way FLTK C++ apps write them
# (`{"&Open", 0, cb}`, trailing fields default to zero); -Wextra
# otherwise flags every single one of them.
CFLAGS  ?= -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -g -Iinclude
# -MMD -MP: emit a .d dependency file per .o listing the headers it
# included, so editing a shared header (e.g. Fl_Valuator.h) correctly
# triggers a rebuild of every .c that includes it, not just .c files
# whose own mtime changed. Without this, `make` silently links stale
# .o files against a changed struct layout -- an ABI mismatch that
# manifests as a segfault calling through a garbage function pointer,
# not a compile error.
DEPFLAGS := -MMD -MP
X11_CFLAGS := $(shell pkg-config --cflags x11 xft)
X11_LIBS   := $(shell pkg-config --libs x11 xft) -lm

CORE_SRCS := \
    src/core/Fl_Widget.c \
    src/core/Fl_Group.c \
    src/core/Fl.c \
    src/core/fl_utf8.c \
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
    src/widgets/Fl_Tabs.c \
    src/widgets/Fl_Scroll.c \
    src/widgets/Fl_Browser_.c \
    src/widgets/Fl_Browser.c \
    src/widgets/Fl_Select_Browser.c \
    src/widgets/Fl_Hold_Browser.c \
    src/widgets/Fl_Multi_Browser.c \
    src/widgets/Fl_Check_Browser.c \
    src/text/Fl_Text_Buffer.c \
    src/text/Fl_Text_Display.c \
    src/text/Fl_Text_Editor.c \
    src/image/Fl_Image.c \
    src/image/Fl_RGB_Image.c \
    src/image/Fl_Pixmap.c \
    src/image/Fl_Bitmap.c \
    src/image/Fl_BMP_Image.c \
    src/image/Fl_GIF_Image.c \
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
    src/valuators/Fl_Roller.c \
    src/valuators/Fl_Value_Input.c \
    src/valuators/Fl_Value_Output.c \
    src/valuators/Fl_Adjuster.c

X11_SRCS := \
    src/backend/x11/fl_x11_window.c \
    src/backend/x11/fl_x11_event.c \
    src/backend/x11/fl_x11_driver.c

# Fl_PNG_Image/Fl_JPEG_Image are only compiled at all when their library
# is found via pkg-config -- see include/cfltk/Fl_PNG_Image.h and
# Fl_JPEG_Image.h for why this is a build-time inclusion decision (no
# external library) rather than an internal #ifdef. Override with
# `make CFLTK_ENABLE_PNG=0` / `CFLTK_ENABLE_JPEG=0` to force-disable
# even when the library is present.
CFLTK_ENABLE_PNG  ?= $(shell pkg-config --exists libpng && echo 1 || echo 0)
CFLTK_ENABLE_JPEG ?= $(shell pkg-config --exists libjpeg && echo 1 || echo 0)

IMG_SRCS :=
IMG_CFLAGS :=
IMG_LIBS :=
ifeq ($(CFLTK_ENABLE_PNG),1)
    IMG_SRCS += src/image/Fl_PNG_Image.c
    IMG_CFLAGS += $(shell pkg-config --cflags libpng) -DCFLTK_HAVE_PNG
    IMG_LIBS += $(shell pkg-config --libs libpng)
endif
ifeq ($(CFLTK_ENABLE_JPEG),1)
    IMG_SRCS += src/image/Fl_JPEG_Image.c
    IMG_CFLAGS += $(shell pkg-config --cflags libjpeg) -DCFLTK_HAVE_JPEG
    IMG_LIBS += $(shell pkg-config --libs libjpeg)
endif

SRCS := $(CORE_SRCS) $(X11_SRCS) $(IMG_SRCS)
OBJS := $(SRCS:.c=.o)

BUILD_DIR := build
LIB := $(BUILD_DIR)/libcfltk.a

.PHONY: all clean examples

all: $(LIB) examples

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) $(X11_CFLAGS) $(IMG_CFLAGS) -c $< -o $@

$(LIB): $(BUILD_DIR) $(OBJS)
	ar rcs $(LIB) $(OBJS)

-include $(OBJS:.o=.d)

examples: $(LIB)
	$(CC) $(CFLAGS) examples/hello/hello.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/hello
	$(CC) $(CFLAGS) examples/buttons/buttons.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/buttons
	$(CC) $(CFLAGS) examples/radio/radio.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/radio
	$(CC) $(CFLAGS) examples/input/input.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/input
	$(CC) $(CFLAGS) examples/menus/menus.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/menus
	$(CC) $(CFLAGS) examples/valuators/valuators.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/valuators
	$(CC) $(CFLAGS) examples/tabs_scroll/tabs_scroll.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/tabs_scroll
	$(CC) $(CFLAGS) examples/browser/browser.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/browser
	$(CC) $(CFLAGS) examples/text_editor/text_editor.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/text_editor
	$(CC) $(CFLAGS) examples/images/images.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/images
	$(CC) $(CFLAGS) $(IMG_CFLAGS) examples/loaders/loaders.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) $(IMG_LIBS) -o $(BUILD_DIR)/loaders

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(LIB)
	rm -rf $(BUILD_DIR)
