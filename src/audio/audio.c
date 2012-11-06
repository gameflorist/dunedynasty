/* audio.c */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include "buildcfg.h"
#include "../os/common.h"
#include "../os/math.h"

#include "audio.h"

#include "../config.h"
#include "../file.h"
#include "../gui/gui.h"
#include "../opendune.h"
#include "../sprites.h"
#include "../string.h"
#include "../table/widgetinfo.h"
#include "../tile.h"
#include "../timer/timer.h"
#include "../tools.h"

bool g_enable_audio;

bool g_enable_music = true;
bool g_enable_effects = true;
bool g_enable_sounds = true;
bool g_enable_voices = true;
bool g_enable_subtitles = false;

float music_volume = 0.85f;
float sound_volume = 0.65f;
float voice_volume = 1.0f;

bool g_opl_mame = true;
char sound_font_path[1024];

enum MusicSet default_music_pack;
static enum MusicID curr_music;
static char music_message[128];

static enum SampleSet s_curr_sample_set = SAMPLESET_INVALID;

static int64_t s_sample_last_played[SAMPLEID_MAX];
static enum SampleID s_voice_queue[256];
static int s_voice_head = 0;
static int s_voice_tail = 0;

void
Audio_DisplayMusicName(void)
{
	GUI_DisplayText(music_message, 5);
}

void
Audio_ScanMusic(void)
{
	bool verbose = false;
	struct stat st;

	for (enum MusicID musicID = MUSIC_LOGOS; musicID < MUSICID_MAX; musicID++) {
		MusicInfo *m = &g_table_music[musicID];

		if (!g_table_music_set[m->music_set].enable)
			m->enable &=~MUSIC_WANT;

		/* External music. */
		if (m->music_set > MUSICSET_DUNE2_C55) {
			char buf[1024];

			snprintf(buf, sizeof(buf), "%s/%s.flac", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;

#ifdef WITH_MAD
			snprintf(buf, sizeof(buf), "%s/%s.mp3", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;
#endif

			snprintf(buf, sizeof(buf), "%s/%s.ogg", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;

			snprintf(buf, sizeof(buf), "%s/%s.AUD", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;

			m->enable &=~MUSIC_FOUND;
			if (verbose) fprintf(stdout, "[missing] %s\n", m->filename);
			continue;

found_song:

			m->enable |= MUSIC_FOUND;
			if (verbose) fprintf(stdout, "[found]   %s\n", buf);
		}
	}

	/* Use default music pack or Adlib if song is otherwise missing. */
	for (int i = 0; g_table_music_cutoffs[i] < MUSICID_MAX; i++) {
		enum MusicID start = g_table_music_cutoffs[i];
		enum MusicID end = g_table_music_cutoffs[i + 1];
		enum MusicSet def = MUSICSET_DUNE2_ADLIB;
		enum MusicID musicID;
		assert(g_table_music[start].music_set == MUSICSET_DUNE2_ADLIB);

		for (musicID = start; musicID < end; musicID++) {
			const enum MusicSet music_set = g_table_music[musicID].music_set;

			if (g_table_music[musicID].enable == MUSIC_ENABLE) {
				break;
			}
			else if ((music_set == default_music_pack) && (g_table_music[musicID].enable & MUSIC_FOUND)) {
				def = music_set;
			}
		}

		if (musicID < end)
			continue;

		for (musicID = start; musicID < end; musicID++) {
			if (g_table_music[musicID].music_set != def)
				continue;

			if (MUSIC_BONUS <= musicID && musicID < MUSIC_ATTACK1)
				continue;

			g_table_music[musicID].enable = MUSIC_ENABLE;
			g_table_music[musicID].volume = max(0.25f, fabsf(g_table_music[musicID].volume));

			if (verbose) fprintf(stdout, "[default] %s\n", g_table_music[musicID].filename);
		}
	}
}

void
Audio_PlayMusic(enum MusicID musicID)
{
	AudioA5_StopMusic();

	if (musicID == MUSIC_STOP)
		return;

	if ((!g_enable_audio) || (!g_enable_music) || (musicID == MUSIC_INVALID))
		return;

	enum MusicID song, end;
	int num_songs = 0;
	int i;

	/* Pick random song between musicID and end. */
	for (i = 0;; i++) {
		if (g_table_music_cutoffs[i] <= musicID && musicID < g_table_music_cutoffs[i + 1]) {
			end = g_table_music_cutoffs[i + 1];
			break;
		}
	}

	for (song = musicID; song < end; song++) {
		if (g_table_music[song].enable == MUSIC_ENABLE)
			num_songs++;
	}

	if (num_songs <= 0)
		return;

	i = Tools_RandomRange(0, num_songs - 1);
	for (song = musicID;; song++) {
		if (g_table_music[song].enable == MUSIC_ENABLE) {
			if (i <= 0)
				break;

			i--;
		}
	}

	MusicInfo *m = &g_table_music[song];
	if (m->music_set <= MUSICSET_DUNE2_C55) {
		if (m->music_set == MUSICSET_DUNE2_ADLIB) {
			AudioA5_InitAdlibMusic(m);
		}
		else {
			AudioA5_InitMidiMusic(m);
		}

		snprintf(music_message, sizeof(music_message), "Playing %s, track %d", m->filename, m->track);
	}
	else {
		AudioA5_InitExternalMusic(m);
		snprintf(music_message, sizeof(music_message), "Playing %s", m->filename);
	}

	curr_music = song;
}

void
Audio_PlayMusicIfSilent(enum MusicID musicID)
{
	if (!Audio_MusicIsPlaying())
		Audio_PlayMusic(musicID);
}

void
Audio_AdjustMusicVolume(bool increase, bool adjust_current_track_only)
{
	if (!(curr_music < MUSICID_MAX))
		return;

	MusicInfo *m = &g_table_music[curr_music];
	float volume;

	/* Adjust single track. */
	if (adjust_current_track_only && (m->music_set > MUSICSET_DUNE2_C55)) {
		m->volume += increase ? 0.05f : -0.05f;
		m->volume = clamp(0.0f, m->volume, 2.0f);

		volume = music_volume * m->volume;
		snprintf(music_message, sizeof(music_message), "Playing %s, volume %.2f x %.2f",
				m->filename, music_volume, m->volume);
	}
	else {
		music_volume += increase ? 0.05f : -0.05f;
		music_volume = clamp(0.0f, music_volume, 1.0f);

		if (m->music_set <= MUSICSET_DUNE2_C55) {
			volume = music_volume;
			snprintf(music_message, sizeof(music_message), "Playing %s, track %d, volume %.2f",
					m->filename, m->track, volume);
		}
		else {
			volume = music_volume * m->volume;
			snprintf(music_message, sizeof(music_message), "Playing %s, volume %.2f x %.2f",
					m->filename, music_volume, m->volume);
		}
	}

	AudioA5_SetMusicVolume(volume);
}

void
Audio_PlayEffect(enum SoundID effectID)
{
	if ((!g_enable_audio) || (!g_enable_effects))
		return;

	AudioA5_PlaySoundEffect(effectID);
}

static char
Audio_GetSamplePrefix(enum SampleSet setID)
{
	switch (g_gameConfig.language) {
		case LANGUAGE_FRENCH:
			return 'F';

		case LANGUAGE_GERMAN:
			return 'G';

		default:
			break;
	}

	switch (setID) {
		case SAMPLESET_HARKONNEN:
			return 'H';

		case SAMPLESET_ATREIDES:
			return 'A';

		case SAMPLESET_ORDOS:
			return 'O';

		case SAMPLESET_BENE_GESSERIT:
			return 'M';

		default:
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
Audio_LoadSampleFromSet(enum SampleSet setID, enum SampleID sampleID)
{
	const SoundData *s = &g_table_voices[sampleID];
	const char *filename;
	char buf[16];

	/* [+-/?]FILENAME. */
	filename = s->string + 1;
	switch (s->string[0]) {
		case '+':
			/* +: common to all houses. */
			if (s_curr_sample_set != SAMPLESET_INVALID)
				return;

			/* +%c: common to all houses, substitute with language prefix. */
			if (s->string[1] == '%') {
				char prefix = Audio_GetSamplePrefix(SAMPLESET_INVALID);
				snprintf(buf, sizeof(buf), s->string + 1, prefix);
				filename = buf;
			}
			break;

		case '-':
			/* -: common to all houses. */
			if (s_curr_sample_set != SAMPLESET_INVALID)
				return;
			break;

		case '/':
			/* /: bene gesserit only (called mercenary in dune 2). */
			if (setID != SAMPLESET_BENE_GESSERIT)
				return;
			break;

		case '?':
			/* ?%c: load as required, substitute with house or language prefix. */
			if (s->string[1] == '%') {
				char prefix = Audio_GetSamplePrefix(setID);
				snprintf(buf, sizeof(buf), s->string + 1, prefix);
				filename = buf;
			}
			break;

		case '%':
			/* %c: substitute with house or language prefix. */
			{
				char prefix = Audio_GetSamplePrefix(setID);
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
Audio_LoadSampleSet(enum SampleSet setID)
{
	if (!g_enable_audio)
		return;

	if (s_curr_sample_set == setID)
		return;

	for (enum SampleID sampleID = 0; sampleID < SAMPLEID_MAX; sampleID++) {
		Audio_LoadSampleFromSet(setID, sampleID);
	}

	s_curr_sample_set = setID;
}

void
Audio_PlaySample(enum SampleID sampleID, int volume, float pan)
{
	if ((!g_enable_audio) || (!g_enable_sounds) || (sampleID == SAMPLE_INVALID))
		return;

	const int64_t curr_ticks = Timer_GetTicks();
	assert(sampleID < SAMPLEID_MAX);

	if (curr_ticks - s_sample_last_played[sampleID] > 6) {
		s_sample_last_played[sampleID] = curr_ticks;
		AudioA5_PlaySample(sampleID, (float)volume / 255.0f, pan);
	}
}

void
Audio_PlaySoundAtTile(enum SoundID soundID, tile32 position)
{
	if (soundID == SOUND_INVALID)
		return;

	assert(soundID < SOUNDID_MAX);

	const enum SampleID sampleID = g_table_voiceMapping[soundID];

	/* Don't play both Adlib and sound blaster effects.
	 *
	 * XXX: extra additional explosion sounds would be nice.
	 */
	if (!g_enable_sounds || (sampleID == SAMPLE_INVALID))
		Audio_PlayEffect(soundID);

	if (g_enable_sounds && (sampleID != SAMPLE_INVALID)) {
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

		Audio_PlaySample(sampleID, volume, pan);
	}
}

void
Audio_PlaySound(enum SoundID soundID)
{
	tile32 tile;

	tile.tile = 0;
	Audio_PlaySoundAtTile(soundID, tile);
}

void
Audio_PlaySoundCutscene(enum SoundID soundID)
{
	if ((!g_enable_sounds) || (soundID == SOUND_INVALID))
		return;

	const enum SampleID sampleID = g_table_voiceMapping[soundID];
	if (sampleID != SAMPLE_INVALID)
		AudioA5_PlaySampleRaw(sampleID, voice_volume, -1000.0f, 1, 11);
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

	if (s->soundId != SOUND_INVALID)
		Audio_PlayEffect(s->soundId);

	if (g_enable_voices) {
		for (int i = 0; i < NUM_SPEECH_PARTS; i++) {
			const enum SampleID sampleID = s->voiceId[i];

			if (sampleID == SAMPLE_INVALID)
				break;

			if (!Audio_QueueVoice(sampleID))
				break;
		}

		Audio_Poll();
	}

	if (g_enable_subtitles) {
		g_viewportMessageText = String_Get_ByIndex(s->messageId);
		g_viewportMessageCounter = 4;
	}
}

bool
Audio_Poll(void)
{
	if ((!g_enable_audio) || (!g_enable_voices))
		return false;

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
