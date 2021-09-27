# dunedynasty-macos
A fork of [Dune Dynasty](http://dunedynasty.sourceforge.net/) that can be built and run on modern Macs, including Apple Silicon (M1) on Big Sur.

## Differences from Dune Dynasty

1. Added more scaling options for UI widgets to support Retina displays. All widget/viewport zoom keys can go to 4x/6x scale respectively. This is also supported by the config file.
2. Added build/usage instructions for macOS.

## Building on macOS

You'll need Homebrew to install the build-time dependencies.

```bash
brew install cmake allegro fluid-synth mad
cmake .
export FLUIDSYNTH_INSTALL_PATH=$(brew info fluid-synth | grep Cellar | head -n 1 | cut -f1 -d" ")
export MAD_INSTALL_PATH=$(brew info mad | grep Cellar | head -n 1 | cut -f1 -d" ")
LIBRARY_PATH="${FLUIDSYNTH_INSTALL_PATH}/lib:${MAD_INSTALL_PATH}/lib" make -j
```

The game executable will be generated at `./dist/dunedynasty`.

## Running the Game

Please refer to [Dune Dynasty](README_dd.txt) documentation for general game-related questions.

## macOS Usage Specifics

The config file is located at `$HOME/Library/Application Support/dunedynasty/dunedynasty.cfg`. For the best experience on Retina displays, it is recommended to set it to something like the following:

```ini
[graphics]
driver=opengl
window_mode=windowed
screen_width=1920
screen_height=1440
menubar_scale=3.00
sidebar_scale=3.00
viewport_scale=4.00
hardware_cursor=1
```
