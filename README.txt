Dune Dynasty
============

Classic Dune. Modern Controls.

About
-----

Dune Dynasty is a continuation of the classic real-time strategy game
Dune II by Westwood Studios.  It is not a remake.  It builds upon the
original game engine as reverse-engineered by the [OpenDUNE] project.

Dune Dynasty features these modern enhancements:

  - Runs natively on Linux and Windows (OpenGL required)
  - High-resolution graphics, including zooming
  - Multiple unit selection with control groups
  - New build queue interface
  - Rally points
  - Multiple sound channels

Plus:

  - Emulated Ad-Lib sound and music playback
  - General MIDI playback using FluidSynth
  - Bug fixes
  - Smoother unit animation
  - Brutal AI mode

Dune Dynasty is licensed under the GNU General Public License version
2.0.  For more information, see the `COPYING` file included with every
release and source download of the game.

[![Screenshot]][Screenshot]


Download
--------

[Download] the source code or Windows binaries from the [SourceForge]
project page.

Read the list of [changes].


Compiling from source
---------------------
(Skip this if you are using precompiled Windows binaries.)

You will need [CMake] and [Allegro 5].  [FluidSynth] is an optional dependency.

    cmake .
    make

Alternatively, you can use an out-of-source build:

    mkdir Build
    cd Build
    cmake ..
    make

The binary will be placed in the dist/ directory.


Starting up
-----------

You will need the *.PAK data files from the EU v1.07 release of Dune II.
Place them into one of the following places:

 1. In a directory named `data` next to the dunedynasty executable.
    This is the simplest option.

 2. In your personal data directory.
    The location depends on your operating system.

    On Unix, this will be:

        ~/.local/share/dunedynasty/data

    On Windows, this will be something like:

        C:\users\nobody\Application Data\Dune Dynasty\data

    or

        C:\users\nobody\AppData\Roaming\Dune Dynasty\data

    or

        C:\Documents and Settings\nobody\Application Data\Dune Dynasty\data

 3. The system-wide directory as configured in the CMake variable
    `$DUNE_DATA_DIR/data`.

Once the data files are in place, you may start the game by running
`dunedynasty.exe` or `dunedynasty`.


Controls
--------

The controls should be similar to most real-time strategy games.
You can finally select multiple units by dragging a rectangle,
or shift clicking.  Right click issues commands on units, and sets
the rally point on buildings.

Keyboard shortcuts are mostly just the first letter of the action.

    A           Attack or Harvest
    M           Move
    G, S        Guard (Stop)
    H           Select construction yard
    P           Place constructed structure

    Ctrl-1      Assign control group 1
    Ctrl-2      Assign control group 2, etc.
    1-0         Jump to control group 1-0
    `           Toggle health bars

    -, +        Zoom in or out
    [, ]        Toggle size of menu and side bars
    Alt-Enter   Toggle windowed mode

    F1          Mentat
    F2          Options
    F3          Click structure icon
    F5          Show current song
    F6          Decrease music volume
    F7          Increase music volume
    F11         Toggle windowed mode
    F12         Save screenshot into data directory

Double tap H or a control group number to centre on the
construction yard or the control group.


Configuration
-------------

The configuration file will be read from one of two places:

 1. In the same directory as the dunedynasty executable.

 2. In a personal data directory.
    On Unix the configuration file is located at:

        ~/.config/dunedynasty/dunedynasty.cfg

    On Windows the configuration file will be located somewhere like:

        C:\users\nobody\Application Data\Dune Dynasty\dunedynasty.cfg

See the sample file `dunedynasty.cfg-sample` for a list of configuration
options.  You can modify the existing `dunedynasty.cfg` file or
replace it with `dunedynasty.cfg-sample`.


General MIDI music
------------------

Dune Dynasty can play MIDI music if it was compiled with FluidSynth support.
You will need to set the `sound_font` path in the configuration file to
an appropriate sound font (.sf2) file, e.g.

    [audio]
    sound_font=/usr/share/sounds/sf2/FluidR3_GM.sf2


External music packs
--------------------

Dune Dynasty can play various external music sets, e.g. music from Dune 2000.
Each of these have their own subdirectory in the dist/ directory.
Instructions are provided in the respective FILELIST.TXT files.

Note that many of the external music sets are provided in .mp3 format.
Currently you must convert them to Ogg Vorbis (.ogg) or FLAC formats.

If you want to disable any music set, edit the [music] section of the
config file.  If a particular track is missing, it will look for a
suitable replacement in the default music pack, or use the Adlib music
if that also fails.  e.g. to play Sega Mega Drive music, and MT-32
rips only as required:

    [music]
    dune2_adlib=0
    dune2_sc55=0
    ...
    dune2_smd=1
    default=fed2k_mt32

In addition, individual songs can be disabled.  e.g. if you want to
include Dune 2000 music, but exclude "Robotix":

    [music/dune2000]
    ROBOTIX=0


Saved games
-----------

Saved games are located in the `save` directory next to `dunedynasty.cfg`.
If no configuration file exists, they will be in placed in a personal
data directory.

On Unix this will be:

    ~/.config/dunedynasty/save

On Windows this will be something like:

    C:\users\nobody\Application Data\Dune Dynasty\save

Saved games from Dune II should work if placed there.


Acknowledgements
----------------

Thank you to:

The OpenDUNE team:

  - Albert Hofkamp (Alberth)
  - Loic Guilloux (glx)
  - Patric Stout (TrueBrain)
  - Steven Noorbergen (Xaroth)

The Allegro 5 developers.

The developers of DOSBox, MAME, ScummVM, Dune Legacy, and everyone else
who worked on the Adlib/OPL/MIDI player code.

Peter, for help on various bits of the code, the music code, and AUDlib.

Nyerguds, for many bug reports and suggestions.

Westwood Studios, for an amazing game!


Author
------

David Wang <dswang@users.sourceforge.net>



[OpenDUNE]: http://www.opendune.org/
[Allegro 5]: http://alleg.sourceforge.net/
[CMake]: http://www.cmake.org/
[FluidSynth]: http://sourceforge.net/apps/trac/fluidsynth/
[Download]: http://sourceforge.net/projects/dunedynasty/files/
[Screenshot]: http://sourceforge.net/projects/dunedynasty/screenshots/screenshot_hark2.png "Screenshot"
[SourceForge]: http://sourceforge.net/projects/dunedynasty/
[changes]: changes.html
