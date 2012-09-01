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
    ALLEGRO_AUDIO_STREAM *stream;
    ALLEGRO_TIMER *timer;
    midi_event *midi_events;
    midi_event *midi_events_pos;
    int ppqn;
};

static bool
init_midi_player(MIDI_PLAYER *pl, const char *sfpath)
{
    pl->settings = new_fluid_settings();
    if (!pl->settings)
        return false;

    // fluid_settings_setstr(pl->settings, "synth.reverb.active", "yes");
    // fluid_settings_setstr(pl->settings, "synth.chorus.active", "no");

    pl->synth = new_fluid_synth(pl->settings);
    if (!pl->synth)
        return false;

    fluid_synth_set_sample_rate(pl->synth, SRATE);

    if (fluid_synth_sfload(pl->synth, sfpath, 1) == -1)
        return false;

    /* Fluidsynth and Allegro both use floating point internally. */
    pl->stream = al_create_audio_stream(FRAG_COUNT, FRAG_SAMPLES, SRATE,
        ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
    if (!pl->stream)
        return false;

    /* Create a timer at some arbitrary rate but don't start it yet. */
    pl->timer = al_create_timer(1000.0);

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
    if (pl->timer)
        al_destroy_timer(pl->timer);
    if (pl->synth)
        delete_fluid_synth(pl->synth);
    if (pl->settings)
        delete_fluid_settings(pl->settings);
    if (pl->midi_events)
        XMIDI::DeleteEventList(pl->midi_events);
    free(pl);
}

ALLEGRO_AUDIO_STREAM *
get_midi_player_audio_stream(MIDI_PLAYER *pl)
{
    return pl->stream;
}

ALLEGRO_TIMER *
get_midi_player_timer(MIDI_PLAYER *pl)
{
    return pl->timer;
}

bool
play_xmidi(MIDI_PLAYER *pl, const char *filename, int track)
{
    FILE *fp;

    stop_midi_player(pl);

    fp = fopen(filename, "rb");
    if (fp) {
        FileDataSource inp(fp);
        XMIDI xmidi(&inp, XMIDI_CONVERT_NOCONVERSION);
        xmidi.retrieve(track, &pl->midi_events, pl->ppqn);
        fclose(fp);
    }

    if (!pl->midi_events)
        return false;

    pl->midi_events_pos = pl->midi_events;

    /* MIDI defaults to 120 BPM. */
    al_set_timer_speed(pl->timer, ALLEGRO_BPM_TO_SECS(120.0 * pl->ppqn));
    al_start_timer(pl->timer);

    return true;
}

void
stop_midi_player(MIDI_PLAYER *pl)
{
    if (pl->timer) {
        al_stop_timer(pl->timer);
    }
    if (pl->midi_events) {
        XMIDI::DeleteEventList(pl->midi_events);
        pl->midi_events = NULL;
        pl->midi_events_pos = NULL;
    }
}

static bool
process_midi_event(MIDI_PLAYER *pl, const midi_event *ev)
{
    uint8_t cmd  = ev->status & 0xf0;
    uint8_t chan = ev->status & 0x0f;
    uint8_t param1 = ev->data[0];
    uint8_t param2 = ev->data[1];

    switch (cmd) {
        case 0x80:  /* Note off */
            fluid_synth_noteoff(pl->synth, chan, param1);
            break;
        case 0x90:  /* Note on */
            fluid_synth_noteon(pl->synth, chan, param1, param2);
            break;
        case 0xA0:  /* Aftertouch */
            break;
        case 0xB0:  /* Control change */
            fluid_synth_cc(pl->synth, chan, param1, param2);
            break;
        case 0xC0:  /* Program change */
            fluid_synth_program_change(pl->synth, chan, param1);
            break;
        case 0xD0:  /* Channel pressure */
            break;
        case 0xE0:  /* Pitch bench */
            fluid_synth_pitch_bend(pl->synth, chan, (param2 << 7) | param1);
            break;
        case 0xF0:  /* SysEx */
            if (param1 == 0x51 && ev->len == 3) {
                /* Set tempo */
                const uint8_t b0 = ev->buffer[0];
                const uint8_t b1 = ev->buffer[1];
                const uint8_t b2 = ev->buffer[2];
                const int usec_per_qn = (b0 << 16) | (b1 << 8) | b2;
                const double usec_per_min = 60e6;
                const double bpm = usec_per_min / usec_per_qn;
                al_set_timer_speed(pl->timer, ALLEGRO_BPM_TO_SECS(bpm * pl->ppqn));
            }
            else if (chan == 0x0f && param1 == 0x2f && param2 == 0x00) {
                /* End of track */
                return false;
            }
            break;
        default:
            fprintf(stderr, "unhandled command: 0x%02X\n", cmd);
            break;
    }

    return true;
}

bool
poll_midi_player_timer(MIDI_PLAYER *pl)
{
    for (;;) {
        midi_event *ev = pl->midi_events_pos;
        if (!ev)
            return false;
        if (ev->time > al_get_timer_count(pl->timer))
            return true;
        if (!process_midi_event(pl, ev)) {
            pl->midi_events_pos = NULL; /* end */
            return false;
        }
        pl->midi_events_pos = pl->midi_events_pos->next;
    }
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
    al_register_event_source(queue, al_get_timer_event_source(get_midi_player_timer(pl)));
    al_register_event_source(queue, al_get_keyboard_event_source());

    if (!play_xmidi(pl, midpath, track)) {
        return 1;
    }

    for (;;) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            if (!poll_midi_player_timer(pl))
                break;
        }
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

/* vim: set sts=4 sw=4 et: */
