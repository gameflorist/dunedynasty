#!/bin/bash -e
echo "Removing existing libs..."
rm -rf ./dist/libs
mkdir ./dist/libs
echo "Copying libs..."
(ldd ./dist/dunedynasty | grep -E 'libfluidsynth' |awk '{if(substr($3,0,1)=="/") print $1,$3}' |sort) |cut -d\  -f2 |
xargs -d '\n' -I{} cp --copy-contents {} ./dist/libs
echo "Patching executable..."
ls -1 ./dist/libs/ | while read file
do
    patchelf --remove-needed $file ./dist/dunedynasty
    patchelf --add-needed ./libs/$file ./dist/dunedynasty
done