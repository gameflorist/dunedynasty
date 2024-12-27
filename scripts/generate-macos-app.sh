#!/bin/sh

CONTENTS=../dist/DuneDynasty.app/Contents

# Clean
rm -rf ../dist/DuneDynasty.app
rm -rf "../dist/Dune Dynasty.app"

# Build file structure
mkdir -p $CONTENTS/MacOS
mkdir $CONTENTS/Resources
mkdir $CONTENTS/libs

# Executable
cp ../dist/dunedynasty $CONTENTS/MacOS/

# Resources
cp -r ../dist/campaign $CONTENTS/Resources/
cp -r ../dist/data $CONTENTS/Resources/
cp -r ../dist/music $CONTENTS/Resources/

# Icon
cp ../src/icon/dune2_icon.icns $CONTENTS/Resources/

# Info.plist
ex $CONTENTS/Info.plist <<eof
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
dylibbundler -of -b -x $CONTENTS/MacOS/dunedynasty -d $CONTENTS/libs/
otool -L $CONTENTS/MacOS/dunedynasty

# Hack, libmad require SDL2 ???
cp -r /opt/local/lib/SDL2.framework $CONTENTS/libs/

# Rename
mv ../dist/DuneDynasty.app "../dist/Dune Dynasty.app"
