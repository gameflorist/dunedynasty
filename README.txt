                Dune Dynasty
                    v1.0

Dune Dynasty is licensed under the GNU General Public License version
2.0.  For more information, see the COPYING file included with every
release and source download of the game.


Compiling
---------

You will need cmake and Allegro 5.  FluidSynth is optional.

    cmake .
    make

The binary will be placed in the bin/ directory.


Starting Up
-----------

You will need to extract the eu v1.07 data files into bin/data/.  The
files should have lower case filenames.

    cd bin/data
    unzip dune2_eu_1.07.zip
    md5sum -c FILELIST.TXT

Give dunedynasty a run to make sure it works.

    cd bin
    ./dunedynasty

The game will create and print out a personal data directory, which is
where it will save your games.  You should also copy the sample config
file into this directory.

    Personal data directory: ~/.local/share/dunedynasty/
    cp bin/dunedynasty.cfg-sample ~/.local/share/dunedynasty/dunedynasty.cfg

If you have FluidSynth installed, you will need to set the sound_font
path in the config file to enable General MIDI.  e.g.

    [audio]
    sound_font=/usr/share/sounds/sf2/FluidR3_GM.sf2

Dune Dynasty also recognises various external music sets, which it can
play instead of the Adlib or General MIDI music.  Each of these have
their own subdirectory in the bin/ directory.  Instructions are
provided in the respective FILELIST.TXT files.

If you want to disable any music set, edit the [music] section of the
config file.  e.g. to disable the Adlib music:

    [music]
    dune2_adlib=0

Once everything is set up to your liking, start dunedynasty again to
play the game!


Controls
--------

The controls should be similar to most real-time strategy games.
Keyboard shortcuts are mostly just the first letter of the action.

    A       Attack or Harvest
    H       Select construction yard
    P       Place constructed structure

    F1      Mentat
    F2      Options

    F5      Show current song
    F6      Decrease music volume
    F7      Increase music volume

    F12     Save screenshot (~/.local/share/dunedynasty/).


Code and Thank Yous
-------------------

The OpenDUNE team:
  Albert Hofkamp (Alberth)
  Loic Guilloux (glx)
  Patric Stout (TrueBrain)
  Steven Noorbergen (Xaroth)

Allegro 5 team.

AdPlug, DOSBox, MAME, ScummVM, Dune Legacy, and everyone else who
worked on the OPL code.

Peter, for help on various bits of the code, the music, and AUDlib.

Westwood Studios, for an amazing game!
