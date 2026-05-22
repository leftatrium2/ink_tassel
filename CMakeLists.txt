cmake_minimum_required(VERSION 3.10)
project(ink_tassel C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Curses REQUIRED)

set(SOURCES
    src/main.c
    src/editor.c
    src/buffer.c
    src/display.c
    src/input.c
    src/menu.c
    src/file_io.c
    src/password.c
    src/i18n.c
)

add_executable(ink_tassel ${SOURCES})

target_include_directories(ink_tassel PRIVATE include)
target_link_libraries(ink_tassel PRIVATE ${CURSES_LIBRARIES})
