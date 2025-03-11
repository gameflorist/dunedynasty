#!/bin/bash -e

echo "Clearing dist folder..."
rm -rf dist
mkdir dist

echo "Performing build..."
cmake -DCMAKE_BUILD_TYPE=Release .
export FLUIDSYNTH_INSTALL_PATH=$(brew info fluid-synth | grep Cellar | head -n 1 | cut -f1 -d" ")
export MAD_INSTALL_PATH=$(brew info mad | grep Cellar | head -n 1 | cut -f1 -d" ")
export ENET_INSTALL_PATH=$(brew info enet | grep Cellar | head -n 1 | cut -f1 -d" ")
LIBRARY_PATH="${FLUIDSYNTH_INSTALL_PATH}/lib:${MAD_INSTALL_PATH}/lib:${ENET_INSTALL_PATH}/lib" make

echo "Copying static files and creating app-structure..."
cp -r ./static/macos/* ./dist
CONTENTS=./dist/DuneDynasty.app/Contents
mkdir -p $CONTENTS/MacOS $CONTENTS/MacOS/libs
cp -r ./static/general/campaign ./static/general/data ./static/general/music ./static/general/gfx ./static/general/dunedynasty.cfg-sample $CONTENTS/Resources/
cp -r ./static/general/licences ./static/general/*.txt ./dist/
mv ./dist/dunedynasty $CONTENTS/MacOS/

echo "Bundling dylibs..."
dylibbundler -od -cd -x $CONTENTS/MacOS/dunedynasty -b -d $CONTENTS/MacOS/libs -p @executable_path/libs/
otool -L $CONTENTS/MacOS/dunedynasty

# Rename bundle with space in the name, as a final step.
# dylibbundler has problems with paths containing spaces
mv ./dist/DuneDynasty.app "./dist/Dune Dynasty.app"