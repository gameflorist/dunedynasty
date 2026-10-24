#!/bin/bash -e

echo "Cleaning up build..."
./scripts/cleanup-build.sh

echo "Performing build..."
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .
ninja

echo "Bundling dlls..."
# Every DLL the executable imports from /mingw32/bin, recursively. (ldd,
# as in the x64 script, cannot read 32-bit executables.)
todo=./dist/dunedynasty.exe
while [ -n "$todo" ]; do
	next=
	for f in $todo; do
		for dll in $(objdump -p "$f" | sed -n 's/^\s*DLL Name: //p'); do
			if [ -e "/mingw32/bin/$dll" ] && [ ! -e "./dist/$dll" ]; then
				cp "/mingw32/bin/$dll" ./dist/
				next="$next ./dist/$dll"
			fi
		done
	done
	todo=$next
done
ls ./dist/*.dll
