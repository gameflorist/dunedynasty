cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(dunedynasty)
set(DUNE_DYNASTY_VERSION 1.3)

set(DUNE_DATA_DIR "." CACHE PATH "Directory containing game data")
option(WITH_FLUIDSYNTH "FluidSynth" ON)

if(NOT DUNE_DATA_DIR)
	set(DUNE_DATA_DIR ".")
endif(NOT DUNE_DATA_DIR)

include(FindPkgConfig)

set(ALLEGRO_VERSION 5.0)
pkg_check_modules(ALLEGRO5 REQUIRED allegro-${ALLEGRO_VERSION})
pkg_check_modules(ALLEGRO5_ACODEC REQUIRED allegro_acodec-${ALLEGRO_VERSION})
pkg_check_modules(ALLEGRO5_AUDIO REQUIRED allegro_audio-${ALLEGRO_VERSION})
pkg_check_modules(ALLEGRO5_IMAGE REQUIRED allegro_image-${ALLEGRO_VERSION})
pkg_check_modules(ALLEGRO5_MEMFILE REQUIRED allegro_memfile-${ALLEGRO_VERSION})
pkg_check_modules(ALLEGRO5_PRIMITIVES REQUIRED allegro_primitives-${ALLEGRO_VERSION})
include_directories(SYSTEM ${ALLEGRO5_INCLUDE_DIRS})

find_package(OpenGL)
include_directories(SYSTEM ${OPENGL_INCLUDE_DIRS})

if(WITH_FLUIDSYNTH)
    pkg_check_modules(FLUIDSYNTH fluidsynth)
    if(FLUIDSYNTH_FOUND)
	include_directories(SYSTEM ${FLUIDSYNTH_INCLUDE_DIRS})
    else(FLUIDSYNTH_FOUND)
	message("FluidSynth not found")
	set(WITH_FLUIDSYNTH OFF)
    endif(FLUIDSYNTH_FOUND)
endif(WITH_FLUIDSYNTH)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "dist")
configure_file("${CMAKE_SOURCE_DIR}/include/buildcfg.h.in" "${PROJECT_BINARY_DIR}/include/buildcfg.h")

# ----------------------------------------------------------------

include(FileList.cmake)
include_directories("include" "${PROJECT_BINARY_DIR}/include")

if(MINGW)
	# Work around cmake bug #12480 when cross-compiling.
	# Fixed in cmake commit 6ddb1e5b
	include(Platform/Windows-windres)
endif(MINGW)

add_executable(dunedynasty WIN32
	${DUNEDYNASTY_SRC_FILES}
	${CMAKE_SOURCE_DIR}/src/icon/icon.rc
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wno-unused-parameter")

target_link_libraries(dunedynasty
	${ALLEGRO5_ACODEC_LDFLAGS}
	${ALLEGRO5_AUDIO_LDFLAGS}
	${ALLEGRO5_IMAGE_LDFLAGS}
	${ALLEGRO5_MEMFILE_LDFLAGS}
	${ALLEGRO5_PRIMITIVES_LDFLAGS}
	${ALLEGRO5_LDFLAGS}
	${OPENGL_LIBRARIES}
	)

if(WITH_FLUIDSYNTH)
    target_link_libraries(dunedynasty
	${FLUIDSYNTH_LDFLAGS}
	)
endif(WITH_FLUIDSYNTH)

install(TARGETS dunedynasty DESTINATION "bin")
install(FILES
	${CMAKE_SOURCE_DIR}/CHANGES.txt
	${CMAKE_SOURCE_DIR}/COPYING
	${CMAKE_SOURCE_DIR}/README.txt
	${CMAKE_SOURCE_DIR}/enhancement.txt
	${CMAKE_SOURCE_DIR}/opendune.txt
	${CMAKE_SOURCE_DIR}/dist/dunedynasty.cfg-sample
	DESTINATION "doc/dunedynasty-${DUNE_DYNASTY_VERSION}")

if(IS_ABSOLUTE ${DUNE_DATA_DIR})
    foreach(subdir data d2tm_adlib d2tm_mt32 d2tm_sc55 dune2000 fed2k_mt32)
	install(FILES ${CMAKE_SOURCE_DIR}/dist/${subdir}/FILELIST.TXT DESTINATION "${DUNE_DATA_DIR}/${subdir}")
    endforeach(subdir)
endif(IS_ABSOLUTE ${DUNE_DATA_DIR})

# ----------------------------------------------------------------

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
