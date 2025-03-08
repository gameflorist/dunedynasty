#!/bin/bash -e

echo "Clearing dist folder..."
rm -rf dist
mkdir dist

echo "Performing build..."
cmake -DCMAKE_BUILD_TYPE=Release .
make

echo "Bundling libraries..."
mkdir dist/libs
(ldd ./dist/dunedynasty | grep -E 'libfluidsynth' |awk '{if(substr($3,0,1)=="/") print $1,$3}' |sort) |cut -d\  -f2 |
xargs -d '\n' -I{} cp --copy-contents {} ./dist/libs
ls -1 ./dist/libs/ | while read file
do
    patchelf --remove-needed $file ./dist/dunedynasty
    patchelf --add-needed ./libs/$file ./dist/dunedynasty
done

echo "Copying static files..."
cp -r ./static/general/* ./dist
cp -r ./static/linux/* ./dist