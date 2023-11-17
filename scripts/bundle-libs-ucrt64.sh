#!/bin/bash -e
echo "Removing existing dlls..."
rm -rf ./dist/*.dll
echo "Copying new dlls..."
ldd ./dist/dunedynasty.exe | grep /ucrt64/bin |awk '{if(substr($3,0,1)=="/") print $1,$3}' |sort |cut -d\  -f2 |
xargs -d '\n' -I{} cp --copy-contents {} ./dist 