#!/bin/bash -e

echo "Cleaning up build..."
./scripts/cleanup-build.sh

echo "Performing build..."
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .
ninja

ntldd ./dist/dunedynasty.exe

echo "Bundling dlls..."
cp /clangarm64/bin/allegro_acodec-5.2.dll ./dist
cp /clangarm64/bin/allegro-5.2.dll ./dist
cp /clangarm64/bin/allegro_audio-5.2.dll ./dist
cp /clangarm64/bin/allegro_image-5.2.dll ./dist
cp /clangarm64/bin/allegro_memfile-5.2.dll ./dist
cp /clangarm64/bin/allegro_primitives-5.2.dll ./dist
cp /clangarm64/bin/libenet-7.dll ./dist
cp /clangarm64/bin/libfluidsynth-3.dll ./dist
cp /clangarm64/bin/libc++.dll ./dist
cp /clangarm64/bin/libmad-0.dll ./dist
cp /clangarm64/bin/libdumb.dll ./dist
cp /clangarm64/bin/libFLAC.dll ./dist
cp /clangarm64/bin/libvorbisfile-3.dll ./dist
cp /clangarm64/bin/libopusfile-0.dll ./dist
cp /clangarm64/bin/libopenal-1.dll ./dist
cp /clangarm64/bin/libwebp-7.dll ./dist
cp /clangarm64/bin/libwinpthread-1.dll ./dist
cp /clangarm64/bin/libreadline8.dll ./dist
cp /clangarm64/bin/libportaudio.dll ./dist
cp /clangarm64/bin/SDL2.dll ./dist
cp /clangarm64/bin/libsndfile-1.dll ./dist
cp /clangarm64/bin/libogg-0.dll ./dist
cp /clangarm64/bin/libopus-0.dll ./dist
cp /clangarm64/bin/libvorbis-0.dll ./dist
cp /clangarm64/bin/libsharpyuv-0.dll ./dist
cp /clangarm64/bin/libtermcap-0.dll ./dist
cp /clangarm64/bin/libmp3lame-0.dll ./dist
cp /clangarm64/bin/libmpg123-0.dll ./dist
cp /clangarm64/bin/libvorbisenc-2.dll ./dist
cp /clangarm64/bin/libglib-2.0-0.dll ./dist
cp /clangarm64/bin/libpcre2-8-0.dll ./dist
cp /clangarm64/bin/libintl-8.dll ./dist
cp /clangarm64/bin/libiconv-2.dll ./dist
cp /clangarm64/bin/libgmodule-2.0-0.dll ./dist