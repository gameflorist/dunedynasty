#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <fluidsynth.h>

#include "xmi/xmidi.h"

int main(int argc, const char **argv)
{
    const char *filename;
    int track;
    FILE *f;

    if (argc < 3) {
        return 1;
    }

    filename = argv[1];
    track = atoi(argv[2]);

    f = fopen(filename, "rb");
    if (!f) {
        return 1;
    }

    FileDataSource input(f);

    XMIDI xmidi(&input, XMIDI_CONVERT_NOCONVERSION);

    fclose(f);

    midi_event *midi_events;
    int ppqn;
    if (!xmidi.retrieve(track, &midi_events, ppqn)) {
        return 1;
    }

    return 0;
}

/* vim: set sts=4 sw=4 et: */
