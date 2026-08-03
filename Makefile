# cfltk - plain Makefile alternative to CMakeLists.txt.
# Builds the core library + the X11 reference backend + examples.

CFLTK_VERSION := 0.1.0

# Install locations, GNU-style: override any of these, or just PREFIX,
# e.g. `make install PREFIX=$HOME/.local` or `make install DESTDIR=/tmp/stage`
# for a staged install (DESTDIR is prepended to every path below but,
# unlike PREFIX, is never baked into the installed .pc file).
PREFIX     ?= /usr/local
LIBDIR     ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
PCDIR      ?= $(LIBDIR)/pkgconfig
DESTDIR    ?=

INSTALL      ?= install
INSTALL_DATA ?= $(INSTALL) -m 644
INSTALL_DIR  ?= $(INSTALL) -d

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
X11_CFLAGS := $(shell pkg-config --cflags x11 xft fontconfig)
X11_LIBS   := $(shell pkg-config --libs x11 xft fontconfig) -lm

CORE_SRCS := \
    src/core/Fl_Widget.c \
    src/core/Fl_Group.c \
    src/core/Fl.c \
    src/core/Fl_Tooltip.c \
    src/core/Fl_Preferences.c \
    src/core/fl_utf8.c \
    src/core/fl_filename.c \
    src/widgets/Fl_Window.c \
    src/widgets/Fl_Double_Window.c \
    src/widgets/Fl_Single_Window.c \
    src/widgets/Fl_Menu_Window.c \
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
    src/widgets/Fl_Pack.c \
    src/widgets/Fl_Tile.c \
    src/widgets/Fl_Wizard.c \
    src/widgets/Fl_Browser_.c \
    src/widgets/Fl_Browser.c \
    src/widgets/Fl_Select_Browser.c \
    src/widgets/Fl_Hold_Browser.c \
    src/widgets/Fl_Multi_Browser.c \
    src/widgets/Fl_Check_Browser.c \
    src/widgets/Fl_File_Browser.c \
    src/text/Fl_Text_Buffer.c \
    src/text/Fl_Text_Display.c \
    src/text/Fl_Text_Editor.c \
    src/image/Fl_Image.c \
    src/image/Fl_RGB_Image.c \
    src/image/Fl_Pixmap.c \
    src/image/Fl_Bitmap.c \
    src/image/Fl_BMP_Image.c \
    src/image/Fl_GIF_Image.c \
    src/image/Fl_Shared_Image.c \
    src/image/Fl_XPM_Image.c \
    src/image/Fl_XBM_Image.c \
    src/menu/Fl_Menu_Item.c \
    src/menu/Fl_Menu_.c \
    src/menu/fl_menu_popup.c \
    src/menu/Fl_Menu_Button.c \
    src/menu/Fl_Choice.c \
    src/menu/Fl_Menu_Bar.c \
    src/draw/fl_draw.c \
    src/draw/fl_symbols.c \
    src/draw/fl_colormap.c \
    src/dialogs/fl_ask.c \
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
    src/valuators/Fl_Adjuster.c \
    src/widgets/Fl_Progress.c \
    src/widgets/Fl_Spinner.c \
    src/widgets/Fl_Clock.c \
    src/widgets/Fl_File_Input.c \
    src/widgets/Fl_Input_Choice.c \
    src/widgets/Fl_Color_Chooser.c \
    src/widgets/Fl_Help_View.c \
    src/dialogs/Fl_Help_Dialog.c

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

.PHONY: all clean examples install uninstall

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
	$(CC) $(CFLAGS) examples/double_window/double_window.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/double_window
	$(CC) $(CFLAGS) examples/layout/layout.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/layout
	$(CC) $(CFLAGS) examples/buttons/buttons.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/buttons
	$(CC) $(CFLAGS) examples/radio/radio.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/radio
	$(CC) $(CFLAGS) examples/input/input.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/input
	$(CC) $(CFLAGS) examples/menus/menus.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/menus
	$(CC) $(CFLAGS) examples/valuators/valuators.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/valuators
	$(CC) $(CFLAGS) examples/tabs_scroll/tabs_scroll.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/tabs_scroll
	$(CC) $(CFLAGS) examples/browser/browser.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/browser
	$(CC) $(CFLAGS) examples/file_browser/file_browser.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/file_browser
	$(CC) $(CFLAGS) examples/text_editor/text_editor.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/text_editor
	$(CC) $(CFLAGS) examples/images/images.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/images
	$(CC) $(CFLAGS) $(IMG_CFLAGS) examples/loaders/loaders.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) $(IMG_LIBS) -o $(BUILD_DIR)/loaders
	$(CC) $(CFLAGS) $(IMG_CFLAGS) examples/shared_image/shared_image.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) $(IMG_LIBS) -o $(BUILD_DIR)/shared_image
	$(CC) $(CFLAGS) examples/tooltip/tooltip.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/tooltip
	$(CC) $(CFLAGS) examples/dialogs/dialogs.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/dialogs
	$(CC) $(CFLAGS) examples/spinner_progress_clock/spinner_progress_clock.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/spinner_progress_clock
	$(CC) $(CFLAGS) examples/input_choice_file/input_choice_file.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/input_choice_file
	$(CC) $(CFLAGS) examples/xpm_xbm/xpm_xbm.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/xpm_xbm
	$(CC) $(CFLAGS) examples/preferences/preferences.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/preferences
	$(CC) $(CFLAGS) examples/color_chooser/color_chooser.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/color_chooser
	$(CC) $(CFLAGS) examples/help_view/help_view.c -L$(BUILD_DIR) -lcfltk $(X11_LIBS) -o $(BUILD_DIR)/help_view

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(LIB)
	rm -rf $(BUILD_DIR)

# Installs the static library, public headers (include/cfltk/*.h, flat,
# no subdirectories to preserve) and a pkg-config file so other
# projects can build against an installed cfltk via
# `pkg-config --cflags --libs cfltk` instead of hand-rolling paths and
# link flags (including the optional libpng/libjpeg link flags, folded
# into the .pc file's Libs.private exactly as this build resolved them
# -- see CFLTK_ENABLE_PNG/CFLTK_ENABLE_JPEG above).
install: $(LIB)
	$(INSTALL_DIR) $(DESTDIR)$(LIBDIR)
	$(INSTALL_DATA) $(LIB) $(DESTDIR)$(LIBDIR)/libcfltk.a
	$(INSTALL_DIR) $(DESTDIR)$(INCLUDEDIR)/cfltk
	$(INSTALL_DATA) include/cfltk/*.h $(DESTDIR)$(INCLUDEDIR)/cfltk/
	$(INSTALL_DIR) $(DESTDIR)$(PCDIR)
	sed \
	    -e 's|@PREFIX@|$(PREFIX)|g' \
	    -e 's|@LIBDIR@|$(LIBDIR)|g' \
	    -e 's|@INCLUDEDIR@|$(INCLUDEDIR)|g' \
	    -e 's|@VERSION@|$(CFLTK_VERSION)|g' \
	    -e 's|@PC_LIBS_PRIVATE@|$(X11_LIBS) $(IMG_LIBS)|g' \
	    cfltk.pc.in > $(DESTDIR)$(PCDIR)/cfltk.pc
	chmod 644 $(DESTDIR)$(PCDIR)/cfltk.pc
	@echo "Installed cfltk $(CFLTK_VERSION) to $(DESTDIR)$(PREFIX)"
	@echo "  (add $(PCDIR) to PKG_CONFIG_PATH if it's not on the default search path)"

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/libcfltk.a
	rm -rf $(DESTDIR)$(INCLUDEDIR)/cfltk
	rm -f $(DESTDIR)$(PCDIR)/cfltk.pc
