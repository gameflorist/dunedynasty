#!/bin/bash -e

echo "Clearing dist folder..."
rm -rf dist
mkdir dist

echo "Performing build..."
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .
ninja

echo "Bundling dlls..."
ldd ./dist/dunedynasty.exe | grep /ucrt64/bin |awk '{if(substr($3,0,1)=="/") print $1,$3}' |sort |cut -d\  -f2 |
xargs -d '\n' -I{} cp --copy-contents {} ./dist

echo "Copying static files..."
cp -r ./static/general/* ./dist