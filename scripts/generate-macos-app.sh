#!/bin/bash -e
CONTENTS=./dist/DuneDynasty.app/Contents

# Clean
rm -rf ./dist/Dune*.app

# Build file structure
mkdir -p $CONTENTS/MacOS $CONTENTS/Resources $CONTENTS/libs

# Move executable
mv ./dist/dunedynasty $CONTENTS/MacOS/

# Move resources
mv ./dist/campaign ./dist/data ./dist/music ./dist/gfx ./dist/dunedynasty.cfg-sample $CONTENTS/Resources/

# Copy icon
cp ./src/icon/dune2_icon.icns $CONTENTS/Resources/

# Create Info.plist
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
	<key>CFBundleExecutable</key>
	<string>dunedynasty</string>
	<key>CFBundleIconFile</key>
	<string>dune2_icon.icns</string>
	<key>CFBundleIdentifier</key>
	<string>io.github.gameflorist.dunedynasty</string>
	<key>LSMinimumSystemVersion</key>
	<string>10.0.0</string>
	<key>NSHighResolutionCapable</key>
	<false/>
</dict>
</plist>" > $CONTENTS/Info.plist

# Bundle dylibs
dylibbundler -of -b -x $CONTENTS/MacOS/dunedynasty -d $CONTENTS/libs/
otool -L $CONTENTS/MacOS/dunedynasty

# Rename bundle with space in the name, as a final step.
# dylibbundler has problems with paths containing spaces
mv ./dist/DuneDynasty.app "./dist/Dune Dynasty.app"
