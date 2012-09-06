#ifndef __included_allegro_midi_h
#define __included_allegro_midi_h

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

typedef struct MIDI_PLAYER MIDI_PLAYER;

extern MIDI_PLAYER *
create_midi_player(const char *sfpath);

extern void
destroy_midi_player(MIDI_PLAYER *pl);

extern ALLEGRO_AUDIO_STREAM *
get_midi_player_audio_stream(MIDI_PLAYER *pl);

extern bool
play_xmidi(MIDI_PLAYER *pl, char *data, unsigned int length, int track);

extern void
stop_midi_player(MIDI_PLAYER *pl);

extern bool
get_midi_playing(MIDI_PLAYER *pl);

extern void
poll_midi_player_fragment(MIDI_PLAYER *pl);

#endif

/* vim: set sts=4 sw=4 et: */
