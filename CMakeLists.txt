cmake_minimum_required(VERSION 3.10)
project(cfltk VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(CFLTK_BACKEND_X11 "Build the Linux/X11 reference backend" ON)
option(CFLTK_BUILD_EXAMPLES "Build the example programs" ON)
option(CFLTK_ENABLE_PNG "Build Fl_PNG_Image (requires libpng); auto-disabled if not found" ON)
option(CFLTK_ENABLE_JPEG "Build Fl_JPEG_Image (requires libjpeg); auto-disabled if not found" ON)

add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
# The last flag isn't a laxness concession: Fl_Menu_Item arrays are meant
# to be written the same terse, positional way FLTK C++ apps write them
# (`{"&Open", 0, cb}` relying on the trailing fields defaulting to zero),
# and -Wextra otherwise flags every single one of them.

# ---------------------------------------------------------------------
# Core library (platform-independent)
# ---------------------------------------------------------------------

set(CFLTK_CORE_SOURCES
    src/core/Fl_Widget.c
    src/core/Fl_Group.c
    src/core/Fl.c
    src/core/Fl_Tooltip.c
    src/core/Fl_Preferences.c
    src/core/fl_utf8.c
    src/core/fl_filename.c
    src/widgets/Fl_Window.c
    src/widgets/Fl_Double_Window.c
    src/widgets/Fl_Single_Window.c
    src/widgets/Fl_Menu_Window.c
    src/widgets/Fl_Box.c
    src/widgets/Fl_Button.c
    src/widgets/Fl_Toggle_Button.c
    src/widgets/Fl_Radio_Button.c
    src/widgets/Fl_Light_Button.c
    src/widgets/Fl_Round_Button.c
    src/widgets/Fl_Check_Button.c
    src/widgets/Fl_Return_Button.c
    src/widgets/Fl_Repeat_Button.c
    src/widgets/Fl_Input.c
    src/widgets/Fl_Float_Input.c
    src/widgets/Fl_Int_Input.c
    src/widgets/Fl_Multiline_Input.c
    src/widgets/Fl_Secret_Input.c
    src/widgets/Fl_Output.c
    src/widgets/Fl_Multiline_Output.c
    src/widgets/Fl_Tabs.c
    src/widgets/Fl_Scroll.c
    src/widgets/Fl_Pack.c
    src/widgets/Fl_Tile.c
    src/widgets/Fl_Wizard.c
    src/widgets/Fl_Browser_.c
    src/widgets/Fl_Browser.c
    src/widgets/Fl_Select_Browser.c
    src/widgets/Fl_Hold_Browser.c
    src/widgets/Fl_Multi_Browser.c
    src/widgets/Fl_Check_Browser.c
    src/widgets/Fl_File_Browser.c
    src/text/Fl_Text_Buffer.c
    src/text/Fl_Text_Display.c
    src/text/Fl_Text_Editor.c
    src/image/Fl_Image.c
    src/image/Fl_RGB_Image.c
    src/image/Fl_Pixmap.c
    src/image/Fl_Bitmap.c
    src/image/Fl_BMP_Image.c
    src/image/Fl_GIF_Image.c
    src/image/Fl_Shared_Image.c
    src/image/Fl_XPM_Image.c
    src/image/Fl_XBM_Image.c
    src/menu/Fl_Menu_Item.c
    src/menu/Fl_Menu_.c
    src/menu/fl_menu_popup.c
    src/menu/Fl_Menu_Button.c
    src/menu/Fl_Choice.c
    src/menu/Fl_Menu_Bar.c
    src/draw/fl_draw.c
    src/draw/fl_symbols.c
    src/draw/fl_colormap.c
    src/dialogs/fl_ask.c
    src/valuators/Fl_Valuator.c
    src/valuators/Fl_Slider.c
    src/valuators/Fl_Fill_Slider.c
    src/valuators/Fl_Hor_Slider.c
    src/valuators/Fl_Hor_Fill_Slider.c
    src/valuators/Fl_Nice_Slider.c
    src/valuators/Fl_Hor_Nice_Slider.c
    src/valuators/Fl_Value_Slider.c
    src/valuators/Fl_Hor_Value_Slider.c
    src/valuators/Fl_Scrollbar.c
    src/valuators/Fl_Dial.c
    src/valuators/Fl_Fill_Dial.c
    src/valuators/Fl_Line_Dial.c
    src/valuators/Fl_Counter.c
    src/valuators/Fl_Simple_Counter.c
    src/valuators/Fl_Roller.c
    src/valuators/Fl_Value_Input.c
    src/valuators/Fl_Value_Output.c
    src/valuators/Fl_Adjuster.c
    src/widgets/Fl_Progress.c
    src/widgets/Fl_Spinner.c
    src/widgets/Fl_Clock.c
    src/widgets/Fl_File_Input.c
    src/widgets/Fl_Input_Choice.c
    src/widgets/Fl_Color_Chooser.c
    src/widgets/Fl_Help_View.c
    src/dialogs/Fl_Help_Dialog.c
)

set(CFLTK_SOURCES ${CFLTK_CORE_SOURCES})

if(CFLTK_BACKEND_X11)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(XFT REQUIRED xft)

    list(APPEND CFLTK_SOURCES
        src/backend/x11/fl_x11_window.c
        src/backend/x11/fl_x11_event.c
        src/backend/x11/fl_x11_driver.c
    )
endif()

# Fl_PNG_Image/Fl_JPEG_Image are only compiled at all when their library
# is found -- see include/cfltk/Fl_PNG_Image.h and Fl_JPEG_Image.h for
# why this is a build-time inclusion decision (no external library) and
# not an internal #ifdef (a NuttX/embedded target may want the whole
# dependency gone, not just stubbed out).
find_package(PkgConfig REQUIRED)
if(CFLTK_ENABLE_PNG)
    pkg_check_modules(PNG libpng)
    if(PNG_FOUND)
        list(APPEND CFLTK_SOURCES src/image/Fl_PNG_Image.c)
    else()
        message(STATUS "libpng not found -- Fl_PNG_Image will not be built")
        set(CFLTK_ENABLE_PNG OFF)
    endif()
endif()
if(CFLTK_ENABLE_JPEG)
    pkg_check_modules(JPEG libjpeg)
    if(JPEG_FOUND)
        list(APPEND CFLTK_SOURCES src/image/Fl_JPEG_Image.c)
    else()
        message(STATUS "libjpeg not found -- Fl_JPEG_Image will not be built")
        set(CFLTK_ENABLE_JPEG OFF)
    endif()
endif()

add_library(cfltk ${CFLTK_SOURCES})
target_include_directories(cfltk PUBLIC include)

if(CFLTK_BACKEND_X11)
    target_include_directories(cfltk PRIVATE ${X11_INCLUDE_DIRS} ${XFT_INCLUDE_DIRS})
    target_link_libraries(cfltk PUBLIC ${X11_LIBRARIES} ${XFT_LIBRARIES})
    target_compile_options(cfltk PRIVATE ${X11_CFLAGS_OTHER} ${XFT_CFLAGS_OTHER})
endif()

if(CFLTK_ENABLE_PNG)
    target_include_directories(cfltk PRIVATE ${PNG_INCLUDE_DIRS})
    target_link_libraries(cfltk PUBLIC ${PNG_LIBRARIES})
    target_compile_options(cfltk PRIVATE ${PNG_CFLAGS_OTHER})
    target_compile_definitions(cfltk PUBLIC CFLTK_HAVE_PNG)
endif()
if(CFLTK_ENABLE_JPEG)
    target_include_directories(cfltk PRIVATE ${JPEG_INCLUDE_DIRS})
    target_link_libraries(cfltk PUBLIC ${JPEG_LIBRARIES})
    target_compile_options(cfltk PRIVATE ${JPEG_CFLAGS_OTHER})
    target_compile_definitions(cfltk PUBLIC CFLTK_HAVE_JPEG)
endif()

find_library(CFLTK_MATH_LIB m)
if(CFLTK_MATH_LIB)
    target_link_libraries(cfltk PUBLIC ${CFLTK_MATH_LIB})
endif()

# ---------------------------------------------------------------------
# Install (`cmake --build . --target install`, or plain `make install`
# in a Makefile-generator build directory)
# ---------------------------------------------------------------------

include(GNUInstallDirs)

install(TARGETS cfltk ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/cfltk DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Mirrors what got linked into the cfltk target above, so an installed
# `pkg-config --libs cfltk` resolves to the same flags this build used
# (X11/Xft always; libpng/libjpeg only if CFLTK_ENABLE_PNG/_JPEG ended
# up ON; libm if found) -- keeps the plain Makefile's `make install`
# and this one producing an equivalent cfltk.pc.
#
# Deliberately using the *_LDFLAGS variables here, not *_LIBRARIES:
# pkg_check_modules() fills _LIBRARIES with bare names ("X11", "Xft",
# not "-lX11"), which is fine for target_link_libraries() above (CMake
# adds the -l itself) but would land in the .pc file's Libs.private as
# invalid, non-flag tokens. _LDFLAGS is already fully formatted
# ("-lX11 ..."), matching what plain `pkg-config --libs` would emit.
set(CFLTK_PC_LIBS_PRIVATE "")
if(CFLTK_BACKEND_X11)
    list(APPEND CFLTK_PC_LIBS_PRIVATE ${X11_LDFLAGS} ${XFT_LDFLAGS})
endif()
if(CFLTK_ENABLE_PNG)
    list(APPEND CFLTK_PC_LIBS_PRIVATE ${PNG_LDFLAGS})
endif()
if(CFLTK_ENABLE_JPEG)
    list(APPEND CFLTK_PC_LIBS_PRIVATE ${JPEG_LDFLAGS})
endif()
if(CFLTK_MATH_LIB)
    list(APPEND CFLTK_PC_LIBS_PRIVATE ${CFLTK_MATH_LIB})
endif()
list(JOIN CFLTK_PC_LIBS_PRIVATE " " CFLTK_PC_LIBS_PRIVATE)

# cfltk.pc.in's placeholders (@PREFIX@ etc.) are named to match the
# plain Makefile's `sed` substitution one-for-one -- see its `install`
# target -- so both build systems generate the same file shape from
# the same template. configure_file(@ONLY) substitutes @VAR@ with the
# CMake variable VAR, hence these short, template-matching names.
set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
set(INCLUDEDIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(VERSION ${PROJECT_VERSION})
set(PC_LIBS_PRIVATE ${CFLTK_PC_LIBS_PRIVATE})
configure_file(cfltk.pc.in cfltk.pc @ONLY)
unset(PREFIX)
unset(LIBDIR)
unset(INCLUDEDIR)
unset(VERSION)
unset(PC_LIBS_PRIVATE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cfltk.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# ---------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------

if(CFLTK_BUILD_EXAMPLES)
    add_executable(hello examples/hello/hello.c)
    target_link_libraries(hello PRIVATE cfltk)

    add_executable(double_window examples/double_window/double_window.c)
    target_link_libraries(double_window PRIVATE cfltk)

    add_executable(layout examples/layout/layout.c)
    target_link_libraries(layout PRIVATE cfltk)

    add_executable(buttons examples/buttons/buttons.c)
    target_link_libraries(buttons PRIVATE cfltk)

    add_executable(radio examples/radio/radio.c)
    target_link_libraries(radio PRIVATE cfltk)

    add_executable(input examples/input/input.c)
    target_link_libraries(input PRIVATE cfltk)

    add_executable(menus examples/menus/menus.c)
    target_link_libraries(menus PRIVATE cfltk)

    add_executable(valuators examples/valuators/valuators.c)
    target_link_libraries(valuators PRIVATE cfltk)

    add_executable(tabs_scroll examples/tabs_scroll/tabs_scroll.c)
    target_link_libraries(tabs_scroll PRIVATE cfltk)

    add_executable(browser examples/browser/browser.c)
    target_link_libraries(browser PRIVATE cfltk)

    add_executable(file_browser examples/file_browser/file_browser.c)
    target_link_libraries(file_browser PRIVATE cfltk)

    add_executable(text_editor examples/text_editor/text_editor.c)
    target_link_libraries(text_editor PRIVATE cfltk)

    add_executable(images examples/images/images.c)
    target_link_libraries(images PRIVATE cfltk)

    add_executable(loaders examples/loaders/loaders.c)
    target_link_libraries(loaders PRIVATE cfltk)

    add_executable(shared_image examples/shared_image/shared_image.c)
    target_link_libraries(shared_image PRIVATE cfltk)

    add_executable(tooltip examples/tooltip/tooltip.c)
    target_link_libraries(tooltip PRIVATE cfltk)

    add_executable(dialogs examples/dialogs/dialogs.c)
    target_link_libraries(dialogs PRIVATE cfltk)

    add_executable(spinner_progress_clock examples/spinner_progress_clock/spinner_progress_clock.c)
    target_link_libraries(spinner_progress_clock PRIVATE cfltk)

    add_executable(input_choice_file examples/input_choice_file/input_choice_file.c)
    target_link_libraries(input_choice_file PRIVATE cfltk)

    add_executable(xpm_xbm examples/xpm_xbm/xpm_xbm.c)
    target_link_libraries(xpm_xbm PRIVATE cfltk)

    add_executable(preferences examples/preferences/preferences.c)
    target_link_libraries(preferences PRIVATE cfltk)

    add_executable(color_chooser examples/color_chooser/color_chooser.c)
    target_link_libraries(color_chooser PRIVATE cfltk)

    add_executable(help_view examples/help_view/help_view.c)
    target_link_libraries(help_view PRIVATE cfltk)
endif()
