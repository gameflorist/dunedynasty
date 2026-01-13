#!/bin/bash -e

echo "Cleaning up build..."
./scripts/cleanup-build.sh

echo "Performing iOS build..."
# iOS build requires Xcode and iOS SDK
# Note: Dependencies (Allegro, FluidSynth, etc.) may need to be built separately for iOS
# or provided as pre-built frameworks. This script provides the basic structure.

# Unset PKG_CONFIG_PATH to prevent finding macOS libraries via pkg-config
# For iOS, Allegro and other dependencies need to be built separately or provided as frameworks
unset PKG_CONFIG_PATH

# Set iOS as the target platform
# Using Xcode generator for iOS builds
# ALLEGRO_IOS_PREFIX should be set by the workflow to point to the iOS-built Allegro installation
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
      -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
      -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="" \
      -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO \
      -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
      -DWITH_ENET=OFF \
      -DWITH_FLUIDSYNTH=OFF \
      -DWITH_MAD=OFF \
      -DWITH_ACODEC=OFF \
      ${ALLEGRO_IOS_PREFIX:+-DALLEGRO_IOS_PREFIX="$ALLEGRO_IOS_PREFIX"} \
      -G Xcode .

# Build using xcodebuild
# Build for iOS device (arm64)
xcodebuild -project dunedynasty.xcodeproj \
           -scheme dunedynasty \
           -configuration Release \
           -sdk iphoneos \
           -arch arm64 \
           CODE_SIGN_IDENTITY="" \
           CODE_SIGNING_REQUIRED=NO \
           CODE_SIGNING_ALLOWED=NO \
           -derivedDataPath build

echo "iOS build completed. Output should be in dist/ or build/"
