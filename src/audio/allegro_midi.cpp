#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <fluidsynth.h>

#include "xmi/xmidi.h"

static fluid_settings_t *settings;
static fluid_synth_t *synth;
static fluid_audio_driver_t *adriver;

static bool
create_synth(void)
{
    settings = new_fluid_settings();
    if (!settings) {
        return false;
    }
    // fluid_settings_setstr(settings, "synth.reverb.active", "yes");
    // fluid_settings_setstr(settings, "synth.chorus.active", "no");
    synth = new_fluid_synth(settings);
    if (!synth) {
        return false;
    }
    adriver = new_fluid_audio_driver(settings, synth);
    if (!adriver) {
        return false;
    }
    return true;
}

static void
delete_synth(void)
{
    if (adriver) {
        delete_fluid_audio_driver(adriver);
        adriver = NULL;
    }
    if (synth) {
        delete_fluid_synth(synth);
        synth = NULL;
    }
    if (settings) {
        delete_fluid_settings(settings);
        settings = NULL;
    }
}

static bool
load_soundfont(const char *sfpath)
{
    int rc = fluid_synth_sfload(synth, sfpath, 1);
    if (rc == -1) {
        return false;
    }
    return true;
}

static midi_event *
load_xmidi(const char *filename, int track, int& ppqn)
{
    midi_event *midi_events = NULL;
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        FileDataSource inp(fp);
        XMIDI xmidi(&inp, XMIDI_CONVERT_NOCONVERSION);
        fclose(fp);
        if (!xmidi.retrieve(track, &midi_events, ppqn)) {
            midi_events = NULL;
        }
    }
    return midi_events;
}

static bool
process_midi_event(const midi_event *ev, fluid_synth_t *synth,
    ALLEGRO_TIMER *timer, int ppqn)
{
    uint8_t cmd  = ev->status & 0xf0;
    uint8_t chan = ev->status & 0x0f;
    uint8_t param1 = ev->data[0];
    uint8_t param2 = ev->data[1];

    switch (cmd) {
        case 0x80:  /* Note off */
            fluid_synth_noteoff(synth, chan, param1);
            break;
        case 0x90:  /* Note on */
            fluid_synth_noteon(synth, chan, param1, param2);
            break;
        case 0xA0:  /* Aftertouch */
            break;
        case 0xB0:  /* Control change */
            fluid_synth_cc(synth, chan, param1, param2);
            break;
        case 0xC0:  /* Program change */
            fluid_synth_program_change(synth, chan, param1);
            break;
        case 0xD0:  /* Channel pressure */
            break;
        case 0xE0:  /* Pitch bench */
            fluid_synth_pitch_bend(synth, chan, (param2 << 7) | param1);
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
                al_set_timer_speed(timer, ALLEGRO_BPM_TO_SECS(bpm * ppqn));
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

int main(int argc, const char **argv)
{
    midi_event *midi_events_start;
    midi_event *midi_events;
    int ppqn;

    if (argc < 3) {
        return 1;
    }

    const char *sfpath = argv[1];
    const char *midpath = argv[2];
    const int track = atoi(argv[3]);

    if (!create_synth()) {
        return 1;
    }
    if (!load_soundfont(sfpath)) {
        return 1;
    }
    midi_events_start = load_xmidi(midpath, track, ppqn);
    if (!midi_events_start) {
        return 1;
    }

    if (!al_init()) {
        return 1;
    }
    al_install_keyboard();
    al_create_display(320, 200);

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_TIMER *timer = al_create_timer(ALLEGRO_BPM_TO_SECS(120.0 * ppqn));

    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    al_start_timer(timer);

    midi_events = midi_events_start;
    while (midi_events) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            while (midi_events && midi_events->time <= ev.timer.count) {
                if (!process_midi_event(midi_events, synth, timer, ppqn)) {
                    midi_events = NULL; /* end */
                    break;
                }
                midi_events = midi_events->next;
            }
        }
        if (ev.type == ALLEGRO_EVENT_KEY_CHAR && ev.keyboard.unichar == 27) {
            break;
        }
    }

    delete_synth();

    XMIDI::DeleteEventList(midi_events);

    return 0;
}

/* vim: set sts=4 sw=4 et: */
