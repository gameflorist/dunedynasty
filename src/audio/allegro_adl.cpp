#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#include "adl/sound_adlib.h"

const int SRATE     = 48000;
const int NUMFRAGS  = 4;
const int FRAGLEN   = 2048;

int main(int argc, const char **argv)
{
    ALLEGRO_AUDIO_STREAM *stream;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_EVENT ev;
    ALLEGRO_FILE *f;
    int track;
    int mame;

    if (argc < 4) {
        return 1;
    }
    if (!al_init()) {
        return 1;
    }
    if (!al_install_audio()) {
        return 1;
    }
    if (!al_reserve_samples(1)) {
        return 1;
    }

    f = al_fopen(argv[1], "rb");
    if (!f) {
        return 1;
    }

    track = atoi(argv[2]);

    mame = atoi(argv[3]);

    SoundAdLibPC adlib(f, SRATE, mame);
    adlib.init();

    al_fclose(f);

    stream = al_create_audio_stream(NUMFRAGS, FRAGLEN, SRATE,
        ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1);
    if (!stream) {
        return 1;
    }
    al_attach_audio_stream_to_mixer(stream, al_get_default_mixer());

    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_audio_stream_event_source(stream));

    adlib.playTrack(track);

    for (;;) {
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
            void *frag = al_get_audio_stream_fragment(stream);

            adlib.callback(&adlib, (SoundAdLibPC::Uint8 *)frag,
                FRAGLEN * al_get_audio_depth_size(ALLEGRO_AUDIO_DEPTH_INT16));

            al_set_audio_stream_fragment(stream, frag);
        }
    }

    return 0;
}

/* vim: set sts=4 sw=4 et: */
