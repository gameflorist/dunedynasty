cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(dunedynasty)

include(FindPkgConfig)

pkg_check_modules(ALLEGRO5 REQUIRED allegro-5.0)
pkg_check_modules(ALLEGRO5_AUDIO REQUIRED allegro_audio-5.0)
pkg_check_modules(ALLEGRO5_IMAGE REQUIRED allegro_image-5.0)
pkg_check_modules(ALLEGRO5_MEMFILE REQUIRED allegro_memfile-5.0)
pkg_check_modules(ALLEGRO5_PRIMITIVES REQUIRED allegro_primitives-5.0)
include_directories(SYSTEM ${ALLEGRO5_INCLUDE_DIRS})

include(FileList.cmake)
include_directories("include")

add_executable(bin/dunedynasty ${DUNEDYNASTY_SRC_FILES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -fomit-frame-pointer")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -Wno-multichar -pedantic")
set(CMAKE_CXX_FLAGS "-W -Wall -Wno-unused-parameter")

target_link_libraries(bin/dunedynasty
	${ALLEGRO5_AUDIO_LDFLAGS}
	${ALLEGRO5_IMAGE_LDFLAGS}
	${ALLEGRO5_MEMFILE_LDFLAGS}
	${ALLEGRO5_PRIMITIVES_LDFLAGS}
	${ALLEGRO5_LDFLAGS}
	)


#add_executable(bin/allegro_adl
#	src/audio/allegro_adl.cpp
#	src/audio/adl/fmopl.cpp
#	src/audio/adl/opl_dosbox.cpp
#	src/audio/adl/opl_mame.cpp
#	src/audio/adl/sound_adlib.cpp
#	)
#
#target_link_libraries(bin/allegro_adl
#	${ALLEGRO5_LDFLAGS}
#	${ALLEGRO5_AUDIO_LDFLAGS}
#	)
