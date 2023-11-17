============
Dune Dynasty
(v1.6.1)
============

About
=====

(This fork is a continuation of the abandoned sourceforge project
[https://sourceforge.net/projects/dunedynasty/] - see development history below for details.)

Dune Dynasty is an enhancement of the classic real-time strategy game Dune II by
Westwood Studios. It's goal is to make Dune II playable on modern systems with
additional features, bugfixes and QoL-improvements. It is not a remake. It
builds upon the original game engine as reverse-engineered by the OpenDUNE
[https://github.com/OpenDUNE/OpenDUNE] project.

Dune Dynasty features these modern enhancements for Dune II:

 * Runs natively on Windows, macOS and Linux (OpenGL or Direct3D)
 * Graphics Enhancements:
   * High-resolution widescreen graphics
   * Separate customizable scaling of menubar, sidebar and map/viewport for
     HiDPI displays
   * Smoother unit animation (optional)
   * High-res overlay (optional)
 * Control Enhancements (Command & Conquer style):
   * Build/order units and structures in scrollable sidepanel
   * Queue building of multiple items
   * Select and command multiple units by drawing a rectangle
   * Select all units of same type in viewport via double-click (or CTRL-LMB)
   * Save/load unit control groups via keyboard hotkeys
   * Set rally points for buildings
   * Pan viewport by keeping RMB pressed
   * Zoom in/out viewport via mousewheel
   * Windows Touchscreen Support [#touchscreen-support]
 * Skirmish and Multiplayer:
   * With 3 additional factions: Fremen, Sardaukar and Mercenaries
   * For up to 6 players/AI
   * Alliances (skirmish only)
   * Random maps (or enter fixed map seed)
   * Settings for starting credits, amount of spice fields and lose condition
 * Sound and Music Enhancements:
   * Emulated AdLib sound and music playback
   * General MIDI playback via FluidSynth [#general-midi-music] (supporting
     SoundFonts)
   * Support for external music sets [#external-music-sets]:
     * Recorded AdLib, MT-32 and SC-55 packs
     * Sega Mega Drive soundtrack (mostly different songs, but also very good!)
     * Dune 2000 game soundtrack
     * Dune (1992) game by Cryo (an excellent, award winning, much more chill
       soundtrack of Dune II's predecessor both as AdLib Gold recording and the
       remastered Spice Opera soundtrack by Exxos)
     * Dune (1984) Original Motion Picture Soundtrack
     * Dune: Part One (2021) Original Motion Picture Soundtrack and "The Dune
       Sketchbook" soundtrack
     * ...with many options for randomly combining them during gameplay
     * ...all playable in in-game jukebox
   * Multiple sound channels
 * Gameplay Enhancements (optional):
   * Health bars
   * Fog of war
   * Brutal AI mode
   * Infantry squad corpses
   * Raise scenario unit cap
   * Start level selection
   * Consistent directional damage (always enabled in multiplayer due to
     balancing)
 * Support for Custom campaigns [#custom-campaigns]:
   * Dune 2 eXtended
     [http://forum.dune2k.com/topic/18360-dune-2-extended-project/]
   * MrFlibble's Alternate Scenarios
     [https://www.mediafire.com/file/9vs75nukou8o3wq/Dune2-MrFlibble%2527sAlternateScenarios.zip/file]
   * Stefan Henriks' Atreides campaign
     [http://arrakis.dune2k.com/downloads.html]
   * Super Dune II [http://forum.dune2k.com/topic/20065-super-dune-ii-classic/]
 * Various bug fixes

A more detailed list of changes by Dune Dynasty and OpenDUNE from the original
Dune II can be found in the file enhancement.txt.

Development History
-------------------

Dune Dynasty was initially developed by David Wang, with the source code hosted
on SourceForge [https://sourceforge.net/projects/dunedynasty/]. Its last
official version was v1.5.7 from 2013. After that David Wang added lots of new
features (mainly multiplayer) up until 2015, but never released a new version.
Later some github-repos emerged with additional fixes and improvements (by
1oom-fork [https://github.com/1oom-fork] and neg3ntropy
[https://github.com/neg3ntropy/dunedynasty]), but again no new release. This
fork is intended to merge these improvements, fix further bugs, add minor
features, and provide new releases (v1.6.0+) with binary download-packages for
Windows, macOS and Linux.

Screenshots
===========

https://github.com/codeflorist/dunedynasty/blob/master/docs/screenshots.jpg

Alternatives
============

Here are some alternatives for enjoying Dune II on modern systems:

 * https://github.com/OpenDUNE/OpenDUNE: Reverse-engineered source port of Dune
   II, upon which Dune Dynasty is based on. It's goal is to keep it as close to
   the original as possible and thus has significantly less features and
   modernizations than Dune Dynasty. OpenDUNE and Dune Dynasty can both read
   original file formats (e.g. save games).
 * https://dunelegacy.sourceforge.net/: A Dune II clone / engine recreation /
   remake with a similar feature-set as Dune Dynasty, but deviating more from
   the original's look and feel. Has multiplayer support and map editor.
 * https://dune2themaker.fundynamic.com/: Another Dune II remake with modern
   features and upscaled graphics. Also deviates quite a bit from the original's
   look and feel.
 * There is also a https://github.com/OpenRA/d2/wiki, an open source engine for
   the early Command & Conquer games.
 * Dune II is also perfectly playable using DOSBox
   [https://dosbox-staging.github.io/].
 * There is a well done Dune II clone for Android on Google Play Store
   [https://play.google.com/store/apps/details?id=de.morphbot.dune].
 * This fork of [https://github.com/YuriyGuts/dunedynasty-macos] has an Apple M1
   Arm executable of v1.5.7.

Dune Dynasty's unique selling points are probably it's faithfulness to the look
and feel of the original (due to it basing on an engine re-creation
[https://github.com/OpenDUNE/OpenDUNE] of the original) combined with many
control modernizations, it's support for fan-generated campaigns
[#custom-campaigns], various music soundtracks [#external-music-sets] and
multiplayer.

Changes
=======

You can find the list of changes between versions in the file CHANGES.txt.

Download
========

The most current Windows, macOS and Linux binaries and source code can be
downloaded from the Github release page:

https://github.com/gameflorist/dunedynasty/releases

Installation
============

You will need the \*.PAK data files from the EU v1.07 release of Dune II. See
the file data/FILELIST.TXT for a complete list of needed files.

Place them into one of the following places:

 1. In a directory named data next to the dunedynasty executable. This is the
    simplest option.

 2. In your personal data directory. The location depends on your operating
    system:
    
    * Windows 64bit:
      C:\users\<your user>\AppData\Roaming\Dune Dynasty\data
    
    * Windows 32bit:
      C:\users\<your user>\Application Data\Dune Dynasty\data
    
    * macOS:
      $HOME/Library/Application Support/dunedynasty/data
    
    * Unix:
      ~/.local/share/dunedynasty/data

Once the data files are in place, you may start the game by running
dunedynasty.exe or dunedynasty.

On Linux you will also have to install the libraries Dune Dynasty depends on:

sudo apt install liballegro5.2 liballegro-acodec5.2 liballegro-image5.2 libenet7 libfluidsynth3 libmad0 libgl1

If libfluidsynth3 is not available on your distribution, try libfluidsynth2
instead.

Configuration
=============

Just as the data-files, the configuration file dunedynasty.cfg will be read from
one of two places:

 1. In the same directory as the dunedynasty executable.

 2. In a personal data directory. This is the default behaviour - meaning
    dunedynasty.cfg will be created here on initial launch. The location depends
    on your operating system:
    
    * Windows 64bit:
      C:\users\<your user>\AppData\Roaming\Dune Dynasty\dunedynasty.cfg
    
    * Windows 32bit:
      C:\users\<your user>\Application Data\Dune Dynasty\dunedynasty.cfg
    
    * macOS:
      $HOME/Library/Application Support/dunedynasty/dunedynasty.cfg
    
    * Unix:
      ~/.config/dunedynasty/dunedynasty.cfg

See the sample file dunedynasty.cfg-sample for a list of configuration options.
You can modify the existing dunedynasty.cfg file or replace it with
dunedynasty.cfg-sample.

Portable mode
-------------

If you place dunedynasty.cfg in the same directory as the dunedynasty
executable, Dune Dynasty will operate in portable mode - keeping everything
(e.g. savegames) inside the install directory.

Video Settings
--------------

Display mode and resolution can be changed in the game's "Options and Extras"
menu. The game initially launches in Fullscreen Window mode (using your
Desktop-resolution).

You might want to increase or decrease the scaling factors of the menubar,
sidebar and viewport. You can do this in-game in the Game Control menu.

Controls
========

The controls should be similar to most real-time strategy games.

You can finally select multiple units by dragging a rectangle, or shift
clicking. You can also select all units of the same type in the current viewport
via a double click or via Ctrl + LMB.

Right click issues commands to units, and sets the rally point on buildings. You
can also change this to Left Click (which is very useful for touchscreen input).

You can pan the viewport by keeping your right mouse-button pressed, and zoom
in/out via the mouse-wheel.

Keyboard shortcuts are mostly just the first letter of the action.

A           Attack or Harvest
H           Harvest
M           Move
G, S        Guard (Stop)
P           Place constructed structure

Ctrl-1      Assign control group 1
Ctrl-2      Assign control group 2, etc.
1-0         Select control group 1-0 (press 2 times to center in viewport)

Ctrl + LMB  Select all units of the same type in current viewport
Double LMB  Select all units of the same type in current viewport
Ctrl + RMB  Set rally points for all similar buildings
Space       Focus viewport on selected structure
Tab         Select next building of the same type
Esc         Pause/Cancel factory task

-, +        Zoom in or out
[, ]        Toggle size of menu and side bars
Alt-Enter   Toggle windowed mode

F1          Mentat
F2          Options
F3          Click structure icon
F4          Select construction yard (press 2 times to center in viewport)
F5          Show current song
F6          Decrease music volume
F7          Increase music volume
F11         Toggle windowed mode
F12         Save screenshot into data directory

There are also keyboard shortcuts for constructing buildings with the
construction yard. They are displayed in the build-panel.

Touchscreen Support
-------------------

Dune Dynasty works very well with touchscreens (e.g. like Microsoft Surface
Pro), since most actions are performed using the left mouse button. You can
perform a right button click via a long (ca. 1 sec.) press. Commands are issued
via the right button by default, but you can change this by setting Control
Style to Left Click in the Game Control options. Then you can quickly select and
command your units via short taps with your finger. To deselect units or
structures perform a long press or simply draw an empty rectangle into the sand.
You can even zoom the viewport with a pinch finger-movement.

Saved games
===========

Saved games are located in the save directory next to dunedynasty.cfg. If no
configuration file exists in the same directory as the executable, they will be
in placed in a personal data directory.

The location depends on your operating system:

 * Windows 64bit:
   C:\users\<your user>\AppData\Roaming\Dune Dynasty\save

 * Windows 32bit:
   C:\users\<your user>\Application Data\Dune Dynasty\save

 * macOS:
   $HOME/Library/Application Support/dunedynasty/save

 * Unix:
   ~/.config/dunedynasty/save

Saved games from Dune II should work if placed there.

Music
=====

Dune Dynasty supports the following music sources:

 * Midi (using your machine's default Midi device)
 * AdLib (built-in emulation)
 * FluidSynth (General Midi via SoundFonts)
 * Various External music sets (see below [#external-music-sets])

By default, Dune Dynasty will randomly mix music from all the available sources.
You can disable any sources in the game's "Options and Extras"-menu (changes
require a restart).

There you will also find a Jukebox to listen to all available songs from all
available sources.

Midi
----

Dune Dynasty can play music via the system MIDI output on Windows and Linux
(ALSA). If you use [Timidity++] as an ALSA sequencer client on Linux you should
start it with smaller buffer sizes to avoid the "drunk drummer" problem:

timidity -iA -B 4,8

FluidSynth
----------

FluidSynth allows playing music via a specified SoundFont. You will need to set
the sound_font path in the configuration file [#configuration] to an appropriate
sound font (.sf2) file, e.g.

[audio]
sound_font=/usr/share/sounds/sf2/FluidR3_GM.sf2

Popular SoundFonts are e.g.:

 * Timbres of Heaven [https://midkar.com/soundfonts/index.html]
 * Arachno Soundfont [https://www.arachnosoft.com/main/soundfont.php]

External music sets
-------------------

Dune Dynasty can play various external music sets, e.g. recordings of the
original PC or Sega Mega Drive soundtrack, music from different Dune games like
Dune 2000 or Cryo's 1992 Dune, or even the motion picture soundtracks.

Each of these have their own subdirectory in the music directory. Instructions
and download/purchase links are provided below and in the respective
FILELIST.TXT files.

Here is a list of supported Music packs:

 * Original Dune II (PC) music:
   
   * rcblanke's SC-55 recording
     [https://www.vogons.org/viewtopic.php?t=33823&start=42]
   * ShaiWa's MT-32 recording [https://dune2k.com/Download/783]
   * Dune II - The Maker AdLib recording
     [https://dune2themaker.fundynamic.com/downloads/mp3adlib.zip]
   * Dune II - The Maker MT-32 recording
     [https://dune2themaker.fundynamic.com/downloads/mp3mt32.zip]
   * Dune II - The Maker SC-55 recording
     [https://dune2themaker.fundynamic.com/downloads/mp3sc55.zip]

 * http://nyerguds.arsaneus-design.com/dune/dunesega/
   Mostly different songs from PC version, but also very good!

 * Dune 2000 game music
   Unfortunately, this game cannot be purchased anymore at the moment.

 * Dune (1992) game (by Cryo) music by Stéphane Picq and Philippe Ulrich:
   The excellent award-winning soundtrack for Dune II's predecessor - the 1992
   Dune game by Cryo. It is very atmospheric, and makes Dune II a much more
   relaxing and chill experience. Two versions are supported:
   
   * AdLib Gold recording by DOS Nostalgia [https://www.dosnostalgia.com/?p=542]
   * Spice Opera by Exxos
     A remastered CD release of the soundtrack. Unfortunately it is not
     available for purchase anymore (but Google might be your friend here).

 * Dune (1984) Original Motion Picture Soundtrack by Toto and Brian Eno
   Seems to be only available on CD. Purchase e.g. from amazon.com
   [https://www.amazon.com/-/de/dp/B000006YDD/] or amazon.de
   [https://www.amazon.de/Dune-Toto/dp/B000006YDD/] and rip files to MP3.

 * Dune: Part One (2021) Original Motion Picture Soundtrack by Hans Zimmer
   Purchase e.g. from amazon.com
   [https://www.amazon.com/music/player/albums/B09F1Y3NCK/] or amazon.de
   [https://www.amazon.de/music/player/albums/B09F2HWHGJ].

 * Dune: Part One (2021) "The Dune Sketchbook" soundtrack by Hans Zimmer
   Purchase e.g. from amazon.com
   [https://www.amazon.com/music/player/albums/B09C3YS6DX] or amazon.de
   [https://www.amazon.de/music/player/albums/B09C3YZZPW].

After installation of a music set, you can check it's availability in the
"Music" section of the game's "Options and Extras" menu. There you can also
enable/disable music sets for random play.

You can also disable individual songs from music sets. You have to do this by
editing your config-file though.

Example: If you want to include Dune 2000 music, but exclude "Robotix":

[music/dune2000]
ROBOTIX=0

Custom campaigns
================

Dune Dynasty supports various fan-made campaigns:

 * Dune 2 eXtended
   [http://forum.dune2k.com/topic/18360-dune-2-extended-project/]
 * MrFlibble's Alternate Scenarios
   [https://www.mediafire.com/file/9vs75nukou8o3wq/Dune2-MrFlibble%2527sAlternateScenarios.zip/file]
 * Stefan Henriks' Atreides campaign [http://arrakis.dune2k.com/downloads.html]
 * Super Dune II [http://forum.dune2k.com/topic/20065-super-dune-ii-classic/]

These should be placed in the existing subdirectories inside the campaign
directory.

Click the arrows next to the "The Building of a Dynasty" subtitle in the main
menu to switch between campaigns.

You can also create your own campaigns. A campaign should consist of a META.INI
file, a REGIONX.INI file for each playable House, where X is the first letter of
the House name, and a complete set of scenarios for each House, named
SCENX001.INI through SCENX022.INI. See Stefan Hendriks' Atreides Campaign
(subfolder shac) as a simple example.

Each campaign can also contain custom balance tweaks, specified in PROFILE.INI
and HOUSE.INI. Please refer to the sample files in the campaign directory for
more information.

Finally, the scenarios can either be distributed as loose INI files or as a
single PAK file. Data stored in PAK files must be listed in META.INI. See
MrFlibble's Alternative Scenarios (subfolder mfas) as a simple example of
scenarios stored in a PAK file.

Development
===========

Compilation
-----------

General Info
------------

The binary will be placed in the dist directory.

The steps below will build the release-version. You can change value of the
DCMAKE_BUILD_TYPE parameter to build different versions, with the possible
options being: Debug, Release, RelWithDebInfo and MinSizeRel.

Windows
-------

 1. Download and install MSYS2 [https://www.msys2.org/#installation].

 2. For a 64bit executable, launch MSYS2 UCRT64 from the startmenu. For 32bit,
    launch MSYS2 MINGW32.

 3. Install dependencies:
    
    For 64bit:
    
    pacman -S mingw-w64-ucrt-x86_64-cmake
    pacman -S mingw-w64-ucrt-x86_64-gcc
    pacman -S mingw-w64-ucrt-x86_64-gdb
    pacman -S mingw-w64-ucrt-x86_64-allegro
    pacman -S mingw-w64-ucrt-x86_64-enet
    pacman -S mingw-w64-ucrt-x86_64-fluidsynth
    pacman -S mingw-w64-ucrt-x86_64-libmad
    
    
    For 32bit:
    
    pacman -S mingw-w64-i686-cmake
    pacman -S mingw-w64-i686-gcc
    pacman -S mingw-w64-i686-gdb
    pacman -S mingw-w64-i686-allegro
    pacman -S mingw-w64-i686-enet
    pacman -S mingw-w64-i686-fluidsynth
    pacman -S mingw-w64-i686-libmad
    

 4. Perform build:
    
    ./scripts/build-windows.sh
    

 5. For packaging, you have to copy all required .dll files to the dist folder.
    To do this, simply call the following script:
    
    For 64bit:
    
    ./scripts/bundle-libs-ucrt64.sh
    
    
    For 32bit:
    
    ./scripts/bundle-libs-mingw32.sh
    

MacOs
-----

 1. Install Homebrew [https://brew.sh/] package manager.

 2. Install dependencies:
    
    brew install cmake allegro fluid-synth mad enet
    

 3. Perform build:
    
    ./scripts/build-macos.sh
    

 4. To package all required dynlibs into the ./dist/libs folder and patch the
    executable, call this script:
    
    ./scripts/bundle-libs-macos.sh    
    
    The script requires the brew package dylibbundler, so install it first:
    
    brew install dylibbundler
    

Linux (Debian, Ubuntu)
----------------------

 1. Install dependencies:
    
    sudo apt -y install build-essential cmake liballegro5-dev libenet-dev libmad0-dev libfluidsynth-dev fluidsynth
    

 2. Perform build:
    
    ./scripts/build-linux.sh
    

 3. When packaging, there is the problem, that the fluidsynth-library is called
    libfluidsynth2 on some distributions, and libfluidsynth3 on others. To
    mitigate, call this script, which will copy the library-file to dist/libs
    and patch the executable to use the inlcuded library instead:
    
    ./scripts/bundle-libs-linux.sh
    

Debugging
---------

You can use the function GUI_DisplayText to display debugging info in the status
bar. E.g.:

GUI_DisplayText("my debug info:%u, my other debug info:%u", 2, my_value, my_other_value);

Acknowledgements
================

Thank you to:

 * David Wang, the original author of Dune Dynasty
 * The OpenDUNE team:
   * Albert Hofkamp (Alberth)
   * Loic Guilloux (glx)
   * Patric Stout (TrueBrain)
   * Steven Noorbergen (Xaroth)
 * The Allegro 5 developers.
 * The developers of DOSBox, MAME, ScummVM, Dune Legacy, and everyone else who
   worked on the AdLib/OPL/MIDI player code.
 * rcblanke, ShaiWa, Nyerguds, Stefan Hendriks and DOS Nostalgia for their
   soundtrack recordings.
 * Peter, for help on various bits of the code, the music code, and AUDlib.
 * Nyerguds, for his Dune II editor.
 * Bug reporters and other improvement suggestions: MrFlibble, Nyerguds, Zocom7,
   EagleEye, gerwin, Leolo, VileRancour, swt83, Paar, Akaine, Wesker.
 * Westwood Studios, for an amazing game!

License
=======

Dune Dynasty is licensed under the GNU General Public License version 2.0. For
more information, see the LICENSE.txt file included with every release and
source download of the game.

Authors
=======

 * David Wang aka wangds [https://github.com/wangds/]: Original author
 * Andrea Ratto aka neg3ntropy [https://github.com/neg3ntropy/]: Compilation
   fixes and graphics updates
 * Zbynek Vyskovsky aka kvr000 [https://github.com/kvr000/]: Compilation fixes
   and other polishing
 * 1oom-fork [https://github.com/1oom-fork/]: Various fixes and new features
 * codeflorist [https://github.com/codeflorist/]: Various fixes, new features
   and maintainer of this fork