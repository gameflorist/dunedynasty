cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(dunedynasty)

include(FileList.cmake)
include_directories("include")

add_executable(bin/dunedynasty ${DUNEDYNASTY_SRC_FILES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -fomit-frame-pointer")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -Wno-multichar -pedantic")

set(ALLEGRO_LIBS "-lallegro_image -lallegro")

add_definitions(${SDL_CFLAGS})
target_link_libraries(bin/dunedynasty ${ALLEGRO_LIBS} ${SDL_LIBS} -lasound)
