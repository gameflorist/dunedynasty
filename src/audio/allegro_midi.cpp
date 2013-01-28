#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <fluidsynth.h>

#include "allegro_midi.h"
#include "xmi/xmidi.h"

#define FRAG_COUNT      (4)
#define FRAG_SAMPLES    (2048)
#define SRATE           (44100)

struct MIDI_PLAYER
{
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_player_t *player;
    ALLEGRO_AUDIO_STREAM *stream;
};

static bool
init_midi_player(MIDI_PLAYER *pl, const char *sfpath)
{
    pl->settings = new_fluid_settings();
    if (!pl->settings)
        return false;

    fluid_settings_setint(pl->settings, "synth.reverb.active", 1);
    fluid_settings_setint(pl->settings, "synth.chorus.active", 0);
    fluid_settings_setnum(pl->settings, "synth.gain", 0.5);

    pl->synth = new_fluid_synth(pl->settings);
    if (!pl->synth)
        return false;

    fluid_synth_set_sample_rate(pl->synth, SRATE);

    if (fluid_synth_sfload(pl->synth, sfpath, 1) == -1)
        return false;

    pl->player = new_fluid_player(pl->synth);
    if (!pl->player)
        return false;

    /* Fluidsynth and Allegro both use floating point internally. */
    pl->stream = al_create_audio_stream(FRAG_COUNT, FRAG_SAMPLES, SRATE,
        ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
    if (!pl->stream)
        return false;

    return true;
}

MIDI_PLAYER *
create_midi_player(const char *sfpath)
{
    MIDI_PLAYER *pl = (MIDI_PLAYER *)calloc(1, sizeof(*pl));
    if (!init_midi_player(pl, sfpath)) {
        destroy_midi_player(pl);
        return NULL;
    }
    return pl;
}

void
destroy_midi_player(MIDI_PLAYER *pl)
{
    if (!pl)
        return;
    if (pl->stream)
        al_destroy_audio_stream(pl->stream);
    if (pl->player)
        delete_fluid_player(pl->player);
    if (pl->synth)
        delete_fluid_synth(pl->synth);
    if (pl->settings)
        delete_fluid_settings(pl->settings);
    free(pl);
}

ALLEGRO_AUDIO_STREAM *
get_midi_player_audio_stream(MIDI_PLAYER *pl)
{
    return pl->stream;
}

bool
play_xmidi(MIDI_PLAYER *pl, char *data, unsigned int length, int track)
{
    bool ok;

    /* stop_midi_player(pl); */

    ok = true;

    {
        BufferDataSource inp(data, length);
        XMIDI xmidi(&inp, XMIDI_CONVERT_NOCONVERSION);

        int len = xmidi.retrieve(track, NULL);
        char buf[len];
        BufferDataSource out(buf, len);
        xmidi.retrieve(track, &out);
        if (fluid_player_add_mem(pl->player, buf, len) != FLUID_OK) {
            ok = false;
        }
    }

    if (ok && fluid_player_play(pl->player) != FLUID_OK) {
        ok = false;
    }

    return ok;
}

void
stop_midi_player(MIDI_PLAYER *pl)
{
    if (pl->player) {
        delete_fluid_player(pl->player);
        pl->player = new_fluid_player(pl->synth);
    }
}

bool
get_midi_playing(MIDI_PLAYER *pl)
{
    return fluid_player_get_status(pl->player) == FLUID_PLAYER_PLAYING;
}

void
poll_midi_player_fragment(MIDI_PLAYER *pl)
{
    void *buf = al_get_audio_stream_fragment(pl->stream);
    if (buf) {
        fluid_synth_write_float(pl->synth, FRAG_SAMPLES,
            buf, 0, 2,  /* left channel */
            buf, 1, 2   /* right channel */
        );
        al_set_audio_stream_fragment(pl->stream, buf);
    }
}

#if 0
int main(int argc, const char **argv)
{
    MIDI_PLAYER *pl;
    ALLEGRO_EVENT_QUEUE *queue;

    if (argc < 3) {
        return 1;
    }

    const char *sfpath = argv[1];
    const char *midpath = argv[2];
    const int track = atoi(argv[3]);

    if (!al_init()) {
        return 1;
    }
    al_install_keyboard();
    al_create_display(320, 200);

    if (!al_install_audio()) {
        return 1;
    }
    al_reserve_samples(0);
    pl = create_midi_player(sfpath);
    if (!pl) {
        return 1;
    }
    al_attach_audio_stream_to_mixer(get_midi_player_audio_stream(pl), al_get_default_mixer());

    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_audio_stream_event_source(pl->stream));
    al_register_event_source(queue, al_get_keyboard_event_source());

    if (!play_xmidi(pl, midpath, track)) {
        return 1;
    }

    for (;;) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
            poll_midi_player_fragment(pl);
        }
        if (ev.type == ALLEGRO_EVENT_KEY_CHAR && ev.keyboard.unichar == 27) {
            break;
        }
    }

    destroy_midi_player(pl);

    return 0;
}
#endif

/* vim: set sts=4 sw=4 et: */
