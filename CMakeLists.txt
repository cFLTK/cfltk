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
    src/menu/Fl_Menu_Item.c
    src/menu/Fl_Menu_.c
    src/menu/fl_menu_popup.c
    src/menu/Fl_Menu_Button.c
    src/menu/Fl_Choice.c
    src/menu/Fl_Menu_Bar.c
    src/draw/fl_draw.c
    src/draw/fl_colormap.c
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

add_library(cfltk ${CFLTK_SOURCES})
target_include_directories(cfltk PUBLIC include)

if(CFLTK_BACKEND_X11)
    target_include_directories(cfltk PRIVATE ${X11_INCLUDE_DIRS} ${XFT_INCLUDE_DIRS})
    target_link_libraries(cfltk PUBLIC ${X11_LIBRARIES} ${XFT_LIBRARIES})
    target_compile_options(cfltk PRIVATE ${X11_CFLAGS_OTHER} ${XFT_CFLAGS_OTHER})
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
endif()
