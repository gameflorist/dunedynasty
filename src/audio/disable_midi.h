#ifndef AUDIO_DISABLE_MIDI_H
#define AUDIO_DISABLE_MIDI_H

typedef void MIDI_PLAYER;

static MIDI_PLAYER *
create_midi_player(const char *sfpath)
{
	(void)sfpath;
	return NULL;
}

static void
destroy_midi_player(MIDI_PLAYER *pl)
{
	(void)pl;
}

static ALLEGRO_AUDIO_STREAM *
get_midi_player_audio_stream(MIDI_PLAYER *pl)
{
	(void)pl;
	return NULL;
}

static bool
play_xmidi(MIDI_PLAYER *pl, char *data, unsigned int length, int track)
{
	(void)pl, (void)data, (void)length, (void)track;
	return false;
}

static void
stop_midi_player(MIDI_PLAYER *pl)
{
	(void)pl;
}

static bool
get_midi_playing(MIDI_PLAYER *pl)
{
	(void)pl;
	return false;
}

static void
poll_midi_player_fragment(MIDI_PLAYER *pl)
{
	(void)pl;
}

#endif
