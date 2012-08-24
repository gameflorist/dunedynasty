/* audio.c */

#include <assert.h>
#include <stdio.h>
#include "../os/math.h"

#include "audio.h"

#include "../config.h"
#include "../file.h"
#include "../gui/gui.h"
#include "../opendune.h"
#include "../sprites.h"
#include "../table/widgetinfo.h"
#include "../tile.h"

float music_volume = 0.85f;
float sound_volume = 0.65f;
float voice_volume = 1.0f;

static enum HouseType s_curr_sample_set = HOUSE_INVALID;

static enum SampleID s_voice_queue[256];
static int s_voice_head = 0;
static int s_voice_tail = 0;

void
Audio_PlayMusic(enum MusicID musicID)
{
	char filename[16];

	if (musicID == MUSIC_INVALID)
		return;

	if (musicID == MUSIC_STOP) {
		AudioA5_StopMusic();
		return;
	}

	const SoundData *m = &g_table_musics[musicID];

	snprintf(filename, sizeof(filename), "%s.ADL", m->string);

	AudioA5_InitMusic(filename, m->variable_04);
}

static char
Audio_GetSamplePrefix(enum HouseType houseID)
{
	switch (g_config.language) {
		case LANGUAGE_FRENCH:
			return 'F';

		case LANGUAGE_GERMAN:
			return 'G';

		default:
			if (houseID < HOUSE_MAX)
				return g_table_houseInfo[houseID].prefixChar;
			break;
	}

	return 'Z';
}

static void
Audio_LoadSample(const char *filename, enum SampleID sampleID)
{
	if (filename == NULL || !File_Exists(filename))
		return;

	const uint8 file_index = File_Open(filename, 1);
	const uint32 file_size = File_GetSize(file_index);

	AudioA5_StoreSample(sampleID, file_index, file_size);
	File_Close(file_index);
}

static void
Audio_LoadSampleForHouse(enum HouseType houseID, enum SampleID sampleID)
{
	const SoundData *s = &g_table_voices[sampleID];
	const char *filename;
	char buf[16];

	/* [+-/?]FILENAME. */
	filename = s->string + 1;
	switch (s->string[0]) {
		case '+':
			/* +: common to all houses. */
			if (s_curr_sample_set != HOUSE_INVALID)
				return;

			/* +%c: common to all houses, substitue with language prefix. */
			if (s->string[1] == '%') {
				char prefix = Audio_GetSamplePrefix(HOUSE_INVALID);
				snprintf(buf, sizeof(buf), s->string + 1, prefix);
				filename = buf;
			}
			break;

		case '-':
			/* -: common to all houses. */
			if (s_curr_sample_set != HOUSE_INVALID)
				return;
			break;

		case '/':
			/* /: mercenary only. */
			if (houseID != HOUSE_MERCENARY)
				return;
			break;

		case '?':
			/* ?%c: load as required, substitute with house or language prefix. */
			if (s->string[1] == '%') {
				char prefix = Audio_GetSamplePrefix(houseID);
				snprintf(buf, sizeof(buf), s->string + 1, prefix);
				filename = buf;
			}
			break;

		case '%':
			/* %c: substitute with house or language prefix. */
			{
				char prefix = Audio_GetSamplePrefix(houseID);
				snprintf(buf, sizeof(buf), s->string, prefix);
				filename = buf;
			}
			break;

		default:
			return;
	}

	Audio_LoadSample(filename, sampleID);
}

void
Audio_LoadSampleSet(enum HouseType houseID)
{
	if (s_curr_sample_set == houseID)
		return;

	for (enum SampleID sampleID = 0; sampleID < SAMPLEID_MAX; sampleID++) {
		Audio_LoadSampleForHouse(houseID, sampleID);
	}

	s_curr_sample_set = houseID;
}

void
Audio_PlaySample(enum SampleID sampleID, int volume, float pan)
{
	if (sampleID == SAMPLE_INVALID)
		return;

	AudioA5_PlaySample(sampleID, (float)volume / 255.0f, pan);
}

void
Audio_PlaySoundAtTile(enum SoundID soundID, tile32 position)
{
	if (soundID == SOUND_INVALID)
		return;

	assert(soundID < SOUNDID_MAX);

	int volume = 255;
	float pan = 0.0f;
	if (position.tile != 0) {
		const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
		const int cx = Tile_GetPackedX(g_viewportPosition) + wi->width / (2 * TILE_SIZE);
		const int cy = Tile_GetPackedY(g_viewportPosition) + wi->height / (2 * TILE_SIZE);
		const uint16 packed = Tile_PackXY(cx, cy);

		const int ux = Tile_GetPosX(position);

		volume = Tile_GetDistancePacked(packed, Tile_PackTile(position));
		if (volume > 64)
			volume = 64;

		volume = 255 - (volume * 255 / 80);
		pan = clamp(-0.5f, 0.05f * (ux - cx), 0.5f);
	}

	const enum SampleID sampleID = g_table_voiceMapping[soundID];

	Audio_PlaySample(sampleID, volume, pan);
}

void
Audio_PlaySound(enum SoundID soundID)
{
	tile32 tile;

	tile.tile = 0;
	Audio_PlaySoundAtTile(soundID, tile);
}

static bool
Audio_QueueVoice(enum SampleID sampleID)
{
	const int index = (s_voice_tail + 1) & 0xFF;

	if (index == s_voice_head)
		return false;

	s_voice_queue[s_voice_tail] = sampleID;
	s_voice_tail = index;
	return true;
}

void
Audio_PlayVoice(enum VoiceID voiceID)
{
	if (voiceID == VOICE_INVALID)
		return;

	if (voiceID == VOICE_STOP) {
		s_voice_head = s_voice_tail;
		return;
	}

	assert(voiceID < VOICEID_MAX);
	const Feedback *s = &g_feedback[voiceID];

	for (int i = 0; i < NUM_SPEECH_PARTS; i++) {
		const enum SampleID sampleID = s->voiceId[i];

		if (sampleID == SAMPLE_INVALID)
			break;

		if (!Audio_QueueVoice(sampleID))
			break;
	}

	Audio_Poll();
}

bool
Audio_Poll(void)
{
	bool playing = AudioA5_PollNarrator();

	if (playing)
		return true;

	if (s_voice_head != s_voice_tail) {
		playing = AudioA5_PlaySample(s_voice_queue[s_voice_head], 1.0f, 0.0f);

		if (playing)
			s_voice_head = (s_voice_head + 1) & 0xFF;
	}

	return playing;
}
