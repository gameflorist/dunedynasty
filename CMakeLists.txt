cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(dunedynasty)

set(DUNE_DATA_DIR "." CACHE PATH "Directory containing game data")
option(WITH_FLUIDSYNTH "FluidSynth" ON)

if(NOT DUNE_DATA_DIR)
	set(DUNE_DATA_DIR ".")
endif(NOT DUNE_DATA_DIR)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDUNE_DATA_DIR='\"${DUNE_DATA_DIR}\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDUNE_DATA_DIR='\"${DUNE_DATA_DIR}\"'")

include(FindPkgConfig)

pkg_check_modules(ALLEGRO5 REQUIRED allegro-5.0)
pkg_check_modules(ALLEGRO5_ACODEC REQUIRED allegro_acodec-5.0)
pkg_check_modules(ALLEGRO5_AUDIO REQUIRED allegro_audio-5.0)
pkg_check_modules(ALLEGRO5_IMAGE REQUIRED allegro_image-5.0)
pkg_check_modules(ALLEGRO5_MEMFILE REQUIRED allegro_memfile-5.0)
pkg_check_modules(ALLEGRO5_PRIMITIVES REQUIRED allegro_primitives-5.0)
include_directories(SYSTEM ${ALLEGRO5_INCLUDE_DIRS})

if(WITH_FLUIDSYNTH)
    pkg_check_modules(FLUIDSYNTH fluidsynth)
    if(FLUIDSYNTH_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWITH_FLUIDSYNTH=1")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITH_FLUIDSYNTH=1")
	include_directories(SYSTEM ${FLUIDSYNTH_INCLUDE_DIRS})
    else(FLUIDSYNTH_FOUND)
	message("FluidSynth not found")
	set(WITH_FLUIDSYNTH OFF)
    endif(FLUIDSYNTH_FOUND)
endif(WITH_FLUIDSYNTH)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "dist")

# ----------------------------------------------------------------

include(FileList.cmake)
include_directories("include")

add_executable(dunedynasty ${DUNEDYNASTY_SRC_FILES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -Wno-multichar -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wno-unused-parameter")

target_link_libraries(dunedynasty
	${ALLEGRO5_ACODEC_LDFLAGS}
	${ALLEGRO5_AUDIO_LDFLAGS}
	${ALLEGRO5_IMAGE_LDFLAGS}
	${ALLEGRO5_MEMFILE_LDFLAGS}
	${ALLEGRO5_PRIMITIVES_LDFLAGS}
	${ALLEGRO5_LDFLAGS}
	)

if(WITH_FLUIDSYNTH)
    target_link_libraries(dunedynasty
	${FLUIDSYNTH_LDFLAGS}
	)
endif(WITH_FLUIDSYNTH)


#add_executable(allegro_adl
#	src/audio/allegro_adl.cpp
#	src/audio/adl/fmopl.cpp
#	src/audio/adl/opl_dosbox.cpp
#	src/audio/adl/opl_mame.cpp
#	src/audio/adl/sound_adlib.cpp
#	)
#
#target_link_libraries(allegro_adl
#	${ALLEGRO5_LDFLAGS}
#	${ALLEGRO5_AUDIO_LDFLAGS}
#	)

#if(FLUIDSYNTH_FOUND)
#    add_executable(allegro_midi
#	src/audio/allegro_midi.cpp
#	src/audio/xmi/xmidi.cpp
#	)
#
#    target_link_libraries(allegro_midi
#	${ALLEGRO5_LDFLAGS}
#	${ALLEGRO5_AUDIO_LDFLAGS}
#	${FLUIDSYNTH_LDFLAGS}
#	)
#endif(FLUIDSYNTH_FOUND)
