#!/bin/bash -e
CONTENTS=../dist/DuneDynasty.app/Contents

# Clean
rm -rf ../dist/Dune*.app

# Build file structure
mkdir -p $CONTENTS/MacOS $CONTENTS/Resources $CONTENTS/libs

# Executable
cp ../dist/dunedynasty $CONTENTS/MacOS/

# Resources
cp -r ../dist/campaign ../dist/data ../dist/music $CONTENTS/Resources/

# Icon
cp ../src/icon/dune2_icon.icns $CONTENTS/Resources/

# Info.plist
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
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
</plist>" > $CONTENTS/Info.plist

# Bundle dylibs
dylibbundler -of -b -x $CONTENTS/MacOS/dunedynasty -d $CONTENTS/libs/
otool -L $CONTENTS/MacOS/dunedynasty

# Hack, fix for libfluidsynth needing SDL2 ???:
# Termination Reason:    Namespace DYLD, Code 1 Library missing
# Library not loaded: @rpath/SDL2.framework/Versions/A/SDL2
# Referenced from: <xxx> /Users/USER/Documents/*/Dune Dynasty.app/Contents/libs/libfluidsynth.3.2.2.dylib
# Reason: tried: '/Users/USER/Documents/dunedynasty/dist/Dune Dynasty.app/Contents/libs/SDL2.framework/Versions/A/SDL2' (no such file)
# (terminated at launch; ignore backtrace)
# Another way to fix is to disable compiling with libfluidsynth
# cp -r /opt/local/lib/SDL2.framework $CONTENTS/libs/

# Rename bundle with space in the name, as a final step.
# dylibbundler has problems with paths containing spaces
mv ../dist/DuneDynasty.app "../dist/Dune Dynasty.app"
