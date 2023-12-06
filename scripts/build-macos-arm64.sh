#!/bin/bash -e
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64 .
export FLUIDSYNTH_INSTALL_PATH=$(brew info fluid-synth | grep Cellar | head -n 1 | cut -f1 -d" ")
export MAD_INSTALL_PATH=$(brew info mad | grep Cellar | head -n 1 | cut -f1 -d" ")
export ENET_INSTALL_PATH=$(brew info enet | grep Cellar | head -n 1 | cut -f1 -d" ")
LIBRARY_PATH="${FLUIDSYNTH_INSTALL_PATH}/lib:${MAD_INSTALL_PATH}/lib:${ENET_INSTALL_PATH}/lib" make