#!/bin/bash -e

echo "Cleaning up build..."
./scripts/cleanup-build.sh

echo "Performing build..."
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_MACOSX_BUNDLE=YES .
export FLUIDSYNTH_INSTALL_PATH=$(brew info fluid-synth | grep Cellar | head -n 1 | cut -f1 -d" ")
export MAD_INSTALL_PATH=$(brew info mad | grep Cellar | head -n 1 | cut -f1 -d" ")
export ENET_INSTALL_PATH=$(brew info enet | grep Cellar | head -n 1 | cut -f1 -d" ")
LIBRARY_PATH="${FLUIDSYNTH_INSTALL_PATH}/lib:${MAD_INSTALL_PATH}/lib:${ENET_INSTALL_PATH}/lib" make

echo "Bundling dylibs..."
CONTENTS=./dist/dunedynasty.app/Contents
dylibbundler -od -cd -x $CONTENTS/MacOS/dunedynasty -b -d $CONTENTS/MacOS/libs -p @executable_path/libs/
otool -L $CONTENTS/MacOS/dunedynasty

# Rename bundle with space in the name, as a final step.
mv ./dist/dunedynasty.app "./dist/Dune Dynasty.app"