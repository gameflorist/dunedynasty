#!/bin/bash -e

echo "Cleaning up build..."
./scripts/cleanup-build.sh

echo "Performing build..."
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .
ninja

echo "Bundling dlls..."
cp /mingw32/bin/allegro_acodec-5.2.dll ./dist
cp /mingw32/bin/allegro-5.2.dll ./dist
cp /mingw32/bin/allegro_audio-5.2.dll ./dist
cp /mingw32/bin/allegro_image-5.2.dll ./dist
cp /mingw32/bin/allegro_memfile-5.2.dll ./dist
cp /mingw32/bin/allegro_primitives-5.2.dll ./dist
cp /mingw32/bin/libenet-7.dll ./dist
cp /mingw32/bin/libfluidsynth-3.dll ./dist
cp /mingw32/bin/libgcc_s_dw2-1.dll ./dist
cp /mingw32/bin/libmad-0.dll ./dist
cp /mingw32/bin/libstdc++-6.dll ./dist
cp /mingw32/bin/libdumb.dll ./dist
cp /mingw32/bin/libFLAC.dll ./dist
cp /mingw32/bin/libvorbisfile-3.dll ./dist
cp /mingw32/bin/libopusfile-0.dll ./dist
cp /mingw32/bin/libopenal-1.dll ./dist
cp /mingw32/bin/libwebp-7.dll ./dist
cp /mingw32/bin/libwinpthread-1.dll ./dist
cp /mingw32/bin/libgomp-1.dll ./dist
cp /mingw32/bin/libreadline8.dll ./dist
cp /mingw32/bin/libportaudio.dll ./dist
cp /mingw32/bin/SDL2.dll ./dist
cp /mingw32/bin/libsndfile-1.dll ./dist
cp /mingw32/bin/libogg-0.dll ./dist
cp /mingw32/bin/libopus-0.dll ./dist
cp /mingw32/bin/libvorbis-0.dll ./dist
cp /mingw32/bin/libsharpyuv-0.dll ./dist
cp /mingw32/bin/libtermcap-0.dll ./dist
cp /mingw32/bin/libmp3lame-0.dll ./dist
cp /mingw32/bin/libmpg123-0.dll ./dist
cp /mingw32/bin/libvorbisenc-2.dll ./dist
cp /mingw32/bin/libglib-2.0-0.dll ./dist
cp /mingw32/bin/libpcre2-8-0.dll ./dist
cp /mingw32/bin/libintl-8.dll ./dist
cp /mingw32/bin/libiconv-2.dll ./dist
cp /mingw32/bin/libgmodule-2.0-0.dll ./dist