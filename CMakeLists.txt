cmake_minimum_required(VERSION 3.10)
project(cfltk C)

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
    src/core/fl_utf8.c
    src/core/fl_filename.c
    src/widgets/Fl_Window.c
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
    src/menu/Fl_Menu_Item.c
    src/menu/Fl_Menu_.c
    src/menu/fl_menu_popup.c
    src/menu/Fl_Menu_Button.c
    src/menu/Fl_Choice.c
    src/menu/Fl_Menu_Bar.c
    src/draw/fl_draw.c
    src/draw/fl_colormap.c
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
# Examples
# ---------------------------------------------------------------------

if(CFLTK_BUILD_EXAMPLES)
    add_executable(hello examples/hello/hello.c)
    target_link_libraries(hello PRIVATE cfltk)

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
endif()
