#!/bin/sh

# clean
rm -rf ../dist/DuneDynasty.app

# Build file structure
mkdir -p ../dist/DuneDynasty.app/Contents/MacOS
mkdir ../dist/DuneDynasty.app/Contents/Resources
mkdir ../dist/DuneDynasty.app/Contents/libs

# Executable
cp ../dist/dunedynasty ../dist/DuneDynasty.app/Contents/MacOS/

# Resources
cp -r ../dist/campaign ../dist/DuneDynasty.app/Contents/Resources/
cp -r ../dist/data ../dist/DuneDynasty.app/Contents/Resources/
cp -r ../dist/music ../dist/DuneDynasty.app/Contents/Resources/

# Icon
cp ../src/icon/dune2_icon.icns ../dist/DuneDynasty.app/Contents/Resources/

# Info.plist
ex ../dist/DuneDynasty.app/Contents/Info.plist <<eof
1 insert
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>dunedynasty</string>
	<key>CFBundleIconFile</key>
	<string>dune2_icon.icns</string>
	<key>CFBundleIdentifier</key>
	<string>dunedynasty.dunedynasty</string>
	<key>NSHighResolutionCapable</key>
    <false/>
</dict>
</plist>
.
xit
eof

# Bundle dylibs
dylibbundler -of -b -x ../dist/DuneDynasty.app/Contents/MacOS/dunedynasty -d ../dist/DuneDynasty.app/Contents/libs/
otool -L ../dist/DuneDynasty.app/Contents/MacOS/dunedynasty

# Hack, libmad require SDL2 ???
cp -r /opt/local/lib/SDL2.framework ../dist/DuneDynasty.app/Contents/libs/
