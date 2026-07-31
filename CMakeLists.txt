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

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# ---------------------------------------------------------------------
# Core library (platform-independent)
# ---------------------------------------------------------------------

set(CFLTK_CORE_SOURCES
    src/core/Fl_Widget.c
    src/core/Fl_Group.c
    src/core/Fl.c
    src/widgets/Fl_Window.c
    src/widgets/Fl_Box.c
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
endif()
