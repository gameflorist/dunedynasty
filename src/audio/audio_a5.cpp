/* audio_a5.cpp */

#include <assert.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_memfile.h>
#include <stdio.h>

#include "audio_a5.h"

#include "adl/sound_adlib.h"

extern "C" {
#include "audio.h"
#include "../common_a5.h"
#include "../file.h"
#include "../house.h"
}

/* Sample instance 0 for narrator voices.
 * Sample instance 1 for acknowledgements.
 * Other instances for general battle sounds.
 */
#define MAX_SAMPLE_INSTANCES 12

/* Music configuration. */
static const int SRATE = 48000;
static const int NUMFRAGS = 4;
static const int FRAGLEN = 2048;
static bool mame = true;

static ALLEGRO_AUDIO_STREAM *s_music_stream;
static SoundAdlibPC *s_adlib;

static ALLEGRO_SAMPLE *s_sample[SAMPLEID_MAX];
static ALLEGRO_SAMPLE_INSTANCE *s_instance[MAX_SAMPLE_INSTANCES];
static ALLEGRO_VOICE *al_voice;
static ALLEGRO_MIXER *al_mixer;

void
AudioA5_Init(void)
{
	if (!al_install_audio()) {
		fprintf(stderr, "al_install_audio() failed.\n");
		g_enable_audio = false;
		return;
	}
	else {
		g_enable_audio = true;
	}

	al_voice = al_create_voice(SRATE, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	al_mixer = al_create_mixer(SRATE, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	al_attach_mixer_to_voice(al_mixer, al_voice);

	s_music_stream = al_create_audio_stream(NUMFRAGS, FRAGLEN, SRATE, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1);
	if (s_music_stream != NULL) {
		al_attach_audio_stream_to_mixer(s_music_stream, al_mixer);

		al_register_event_source(g_a5_input_queue, al_get_audio_stream_event_source(s_music_stream));
	}

	for (int i = 0; i < MAX_SAMPLE_INSTANCES; i++) {
		s_instance[i] = al_create_sample_instance(NULL);
		al_attach_sample_instance_to_mixer(s_instance[i], al_mixer);
	}
}

void
AudioA5_Uninit(void)
{
	for (int sampleID = 0; sampleID < SAMPLEID_MAX; sampleID++) {
		al_destroy_sample(s_sample[sampleID]);
		s_sample[sampleID] = NULL;
	}

	for (int i = 0; i < MAX_SAMPLE_INSTANCES; i++) {
		al_destroy_sample_instance(s_instance[i]);
		s_instance[i] = NULL;
	}

	delete s_adlib;
	al_destroy_audio_stream(s_music_stream);
	al_destroy_mixer(al_mixer);
	al_destroy_voice(al_voice);
	al_uninstall_audio();
}

void
AudioA5_InitMusic(const char *filename, int track)
{
	if (s_music_stream == NULL)
		return;

	uint16 file_index = File_Open(filename, 1);
	if (file_index == FILE_INVALID)
		return;

	uint32 length = File_GetSize(file_index);

	void *buf = malloc(length);
	if (buf == NULL) {
		File_Close(file_index);
		return;
	}

	File_Read(file_index, buf, length);
	File_Close(file_index);

	ALLEGRO_FILE *f = al_open_memfile(buf, length, "r");
	delete s_adlib;
	s_adlib = new SoundAdlibPC(f, SRATE, mame);
	al_fclose(f);

	s_adlib->playTrack(track);
	al_set_audio_stream_gain(s_music_stream, music_volume);
}

void
AudioA5_StopMusic(void)
{
	if (s_adlib != NULL)
		s_adlib->haltTrack();
}

void
AudioA5_PollMusic(void)
{
	if (s_music_stream == NULL || s_adlib == NULL)
		return;

	void *frag = al_get_audio_stream_fragment(s_music_stream);
	if (frag == NULL)
		return;

	s_adlib->callback(s_adlib, (SoundAdlibPC::Uint8 *)frag,
			FRAGLEN * al_get_audio_depth_size(ALLEGRO_AUDIO_DEPTH_INT16));

	al_set_audio_stream_fragment(s_music_stream, frag);
}

bool
AudioA5_MusicIsPlaying(void)
{
	if (s_adlib == NULL)
		return false;

	return (s_adlib->isPlaying());
}

void
AudioA5_PlaySoundEffect(enum SoundID effectID)
{
	if (s_adlib != NULL)
		s_adlib->playSoundEffect(effectID);
}

void
AudioA5_StoreSample(enum SampleID sampleID, uint8 file_index, uint32 file_size)
{
	char header[0x1A];
	char ignore1;
	unsigned int size = 0;
	unsigned char rate;
	char ignore2;
	uint8 *data = (uint8 *)al_malloc(file_size - 32);

	File_Read(file_index, header, 0x1A); /* "Creative Voice File..." */
	File_Read(file_index, &ignore1, 1); /* block type */
	File_Read(file_index, &size, 3);
	File_Read(file_index, &rate, 1);
	File_Read(file_index, &ignore2, 1);
	File_Read(file_index, data, file_size - 32);

	unsigned int freq = 1000000 / (256 - rate);
	ALLEGRO_AUDIO_DEPTH depth = ALLEGRO_AUDIO_DEPTH_UINT8;
	ALLEGRO_CHANNEL_CONF chan_conf = ALLEGRO_CHANNEL_CONF_1;

	al_destroy_sample(s_sample[sampleID]);
	s_sample[sampleID] = al_create_sample(data, size - 2, freq, depth, chan_conf, true);
}

bool
AudioA5_PlaySample(enum SampleID sampleID, float volume, float pan)
{
	int idx_start, idx_end;
	float gain;

	if (s_sample[sampleID] == NULL)
		return true;

	if ((SAMPLE_VOICE_FRAGMENT_ENEMY <= sampleID && sampleID <= SAMPLE_VOICE_FRAGMENT_YOUR_NEXT_CONQUEST) ||
	    (SAMPLE_INTRO_THREE_HOUSES_FIGHT <= sampleID && sampleID <= SAMPLE_INTRO_YOUR) ||
	    (sampleID == SAMPLE_RADAR_STATIC)) {
		idx_start = 0;
		idx_end = 0;
		gain = voice_volume * volume;
		pan = ALLEGRO_AUDIO_PAN_NONE;
	}
	else if (SAMPLE_AFFIRMATIVE <= sampleID && sampleID <= SAMPLE_MOVING_OUT) {
		idx_start = 1;
		idx_end = 1;
		gain = sound_volume * volume;
	}
	else {
		idx_start = 2;
		idx_end = MAX_SAMPLE_INSTANCES - 1;
		gain = sound_volume * volume;
	}

	for (int i = idx_start; i <= idx_end; i++) {
		ALLEGRO_SAMPLE_INSTANCE *si = s_instance[i];

		if (al_get_sample_instance_playing(si))
			continue;

		if (!al_set_sample(si, s_sample[sampleID]))
			continue;

		al_set_sample_instance_gain(si, gain);
		al_set_sample_instance_pan(si, pan);
		al_play_sample_instance(si);
		return true;
	}

	return false;
}

bool
AudioA5_PollNarrator(void)
{
	return (al_get_sample_instance_playing(s_instance[0]));
}
