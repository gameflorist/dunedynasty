/* audio.c */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
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
#include "../table/locale.h"
#include "../table/widgetinfo.h"
#include "../tile.h"
#include "../timer/timer.h"
#include "../tools/coord.h"
#include "../tools/random_general.h"
#include "../tools/random_lcg.h"

bool g_enable_audio;

bool g_enable_music = true;
enum SoundEffectSources g_enable_sound_effects = SOUNDEFFECTS_SYNTH_AND_SAMPLES;
bool g_enable_voices = true;
bool g_enable_subtitles = false;

float music_volume = 0.85f;
float sound_volume = 0.65f;
float voice_volume = 1.0f;

bool g_opl_mame = true;
char sound_font_path[1024];

enum MusicSet default_music_pack;
static MusicInfo *curr_music;
char music_message[128];

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
	const bool verbose = false;
	struct stat st;

	for (enum MusicID musicID = MUSIC_LOGOS; musicID < MUSICID_MAX; musicID++) {
		MusicList *l = &g_table_music[musicID];

		l->count = 0;
		l->count_default = 0;
		for (int s = 0; s < l->length; s++) {
			MusicInfo *m = &l->song[s];

			if (!g_table_music_set[m->music_set].enable)
				m->enable &=~MUSIC_WANT;

			if (m->music_set <= MUSICSET_FLUIDSYNTH)
				goto count_song;

			/* External music. */
			char buf[1024];

#ifdef WITH_ACODEC
			snprintf(buf, sizeof(buf), "%s/%s.flac", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;
#endif

#ifdef WITH_MAD
			snprintf(buf, sizeof(buf), "%s/%s.mp3", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;
#endif

#ifdef WITH_ACODEC
			snprintf(buf, sizeof(buf), "%s/%s.ogg", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;
#endif

#ifdef WITH_AUD
			snprintf(buf, sizeof(buf), "%s/%s.AUD", g_dune_data_dir, m->filename);
			if (stat(buf, &st) == 0)
				goto found_song;
#endif

			m->enable &=~MUSIC_FOUND;
			if (verbose) fprintf(stdout, "[missing] %s\n", m->filename);
			continue;

found_song:

			m->enable |= MUSIC_FOUND;

count_song:

			if (m->enable & MUSIC_FOUND) {
				l->count_found++;

				if ((m->enable & MUSIC_ENABLE) == MUSIC_ENABLE)
					l->count++;

				if (m->music_set == default_music_pack)
					l->count_default++;
			}

			if (verbose) fprintf(stdout, "[found]   %s\n", m->filename);
		}
	}
}

static void
Audio_PlayMusicGroup(enum MusicID musicID, bool respect_want_setting)
{
	enum MusicID start = musicID;
	enum MusicID end = musicID;
	int num_songs = 0;
	int num_songs_default = 0;

	if (musicID == MUSIC_RANDOM_IDLE) {
		start = MUSIC_IDLE1;
		end = MUSIC_BONUS;
	}
	else if (musicID == MUSIC_RANDOM_ATTACK) {
		start = MUSIC_ATTACK1;
		end = MUSIC_ATTACK6;
	}

	for (enum MusicID m = start; m <= end; m++) {
		if (respect_want_setting) {
			num_songs += g_table_music[m].count;
		}
		else {
			num_songs += g_table_music[m].count_found;
		}

		num_songs_default += g_table_music[m].count_default;
	}

	/* If there are no attack songs, as in the Sega Mega Drive and Amiga
	 * versions of Dune II, do not switch songs for uninterrupted music.
	 */
	if ((musicID == MUSIC_RANDOM_ATTACK) && (num_songs <= 0))
		return;

	AudioA5_StopMusic();

	if (musicID == MUSIC_STOP) {
		curr_music = NULL;
		music_message[0] = '\0';
		return;
	}

	if ((!g_enable_audio) || (!g_enable_music) || (musicID == MUSIC_INVALID))
		return;

	int r;

	/* Pick random song between musicID and end. */
	enum MusicFlags check_flags = MUSIC_FOUND;
	enum MusicSet check_music_pack = MUSICSET_INVALID;

	if (num_songs > 0) {
		if (respect_want_setting)
			check_flags = MUSIC_ENABLE;

		r = Tools_RandomLCG_Range(0, num_songs - 1);
	}
	else if (num_songs_default > 0) {
		check_music_pack = default_music_pack;
		r = Tools_RandomLCG_Range(0, num_songs_default - 1);
	}
	else {
		check_music_pack = MUSICSET_DUNE2_ADLIB;

		if (musicID == MUSIC_RANDOM_IDLE) r = Tools_RandomLCG_Range(0, 9 - 1);
		else if (musicID == MUSIC_RANDOM_ATTACK) r = Tools_RandomLCG_Range(0, 6 - 1);
		else r = 0;
	}

	MusicInfo *m = NULL;
	const MusicList *l = NULL;
	for (musicID = start; musicID <= end && (m == NULL); musicID++) {
		l = &g_table_music[musicID];

		for (int s = 0; s < l->length; s++) {
			const bool enable =
				((l->song[s].enable & check_flags) == check_flags) &&
				((l->song[s].music_set == check_music_pack) || (check_music_pack == MUSICSET_INVALID));

			if (enable) {
				if (r <= 0) {
					m = &l->song[s];
					break;
				}

				r--;
			}
		}
	}

	Audio_PlayMusicFile(l, m);
}

void
Audio_PlayMusicFile(const MusicList *l, MusicInfo *m)
{
	if (!g_enable_audio)
		return;

	if (m->music_set <= MUSICSET_FLUIDSYNTH) {
		AudioA5_InitInternalMusic(m);
	}
	else {
		AudioA5_InitExternalMusic(m);
	}

	curr_music = m;

	if (m->songname != NULL) {
		snprintf(music_message, sizeof(music_message), "Playing %s, %s",
				g_table_music_set[m->music_set].name, m->songname);
	}
	else if (l->songname != NULL) {
		snprintf(music_message, sizeof(music_message), "Playing %s, %s",
				g_table_music_set[m->music_set].name, l->songname);
	}
	else {
		snprintf(music_message, sizeof(music_message), "Playing %s", m->filename);
	}
}

void
Audio_PlayMusic(enum MusicID musicID)
{
	Audio_PlayMusicGroup(musicID, true);
}

void
Audio_PlayMusicIfSilent(enum MusicID musicID)
{
	if (!Audio_MusicIsPlaying())
		Audio_PlayMusic(musicID);
}

void
Audio_PlayMusicNextInSequence(void)
{
	if (curr_music == NULL)
		return;

	/* Play a song from the next group, otherwise we end up hearing
	 * the same song when browsing the gallery.  For IDLE_OTHER, since
	 * the songs are different, we play the next song in the list.
	 */
	bool curr_found = false;
	for (enum MusicID musicID = MUSIC_LOGOS; musicID < MUSICID_MAX; musicID++) {
		const MusicList *l = &g_table_music[musicID];

		for (int s = 0; s < l->length; s++) {
			MusicInfo *m = &l->song[s];

			if (!(m->enable & MUSIC_FOUND))
				continue;

			if (m == curr_music) {
				curr_found = true;

				if (musicID != MUSIC_IDLE_OTHER) {
					musicID++;

					if (musicID >= MUSICID_MAX)
						musicID = MUSIC_LOGOS;

					Audio_PlayMusicGroup(musicID, false);
					return;
				}
			}
			else if (curr_found) {
				Audio_PlayMusicFile(l, m);
				return;
			}
		}
	}
}

void
Audio_StopMusicUnlessMenu(void)
{
	if (curr_music == NULL)
		return;

	const MusicList *l = &g_table_music[MUSIC_MAIN_MENU];
	for (int i = 0; i < l->length; i++) {
		if (curr_music == &l->song[i])
			return;
	}

	AudioA5_StopMusic();
}

void
Audio_AdjustMusicVolume(float delta, bool adjust_current_track_only)
{
	if (curr_music == NULL)
		return;

	MusicInfo *m = curr_music;
	float volume;

	/* Adjust single track. */
	if (adjust_current_track_only && (m->music_set > MUSICSET_FLUIDSYNTH)) {
		m->volume += delta;
		m->volume = clamp(0.0f, m->volume, 2.0f);

		volume = music_volume * m->volume;
	}
	else {
		music_volume += delta;
		music_volume = clamp(0.0f, music_volume, 1.0f);

		if (m->music_set <= MUSICSET_FLUIDSYNTH) {
			volume = music_volume;
		}
		else {
			volume = music_volume * m->volume;
		}
	}

	char *str = strstr(music_message, ", vol");
	if (str == NULL)
		str = music_message + strlen(music_message);

	snprintf(str, sizeof(music_message) - (str - music_message), ", vol %.2f", music_volume);
	AudioA5_SetMusicVolume(volume);
}

void
Audio_PlayEffect(enum SoundID effectID)
{
	if ((!g_enable_audio) || (g_enable_sound_effects == SOUNDEFFECTS_NONE))
		return;

	AudioA5_PlaySoundEffect(effectID);
}

static char
Audio_GetSamplePrefix(enum SampleSet setID)
{
	if (g_table_languageInfo[g_gameConfig.language].sample_prefix != '\0')
		return g_table_languageInfo[g_gameConfig.language].sample_prefix;

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

	const uint8 file_index = File_Open(filename, FILE_MODE_READ);
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
			/* /: Bene Gesserit only (called mercenary in Dune II). */
			/* if (setID != SAMPLESET_BENE_GESSERIT) return; */
			if (s_curr_sample_set != SAMPLESET_INVALID)
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

	/* Initialisation. */
	if (setID == SAMPLESET_INVALID) {
		Audio_LoadSample(g_table_voices[SAMPLE_RADAR_STATIC].string + 1, SAMPLE_RADAR_STATIC);
	}

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
	if ((!g_enable_audio) || (sampleID == SAMPLE_INVALID) ||
			(g_enable_sound_effects == SOUNDEFFECTS_NONE ||
			 g_enable_sound_effects == SOUNDEFFECTS_SYNTH_ONLY))
		return;

	const int64_t curr_ticks = Timer_GetTicks();
	assert(sampleID < SAMPLEID_MAX);

	if (curr_ticks - s_sample_last_played[sampleID] > 8) {
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
	bool play_synth = (sampleID == SAMPLE_INVALID);

	switch (g_enable_sound_effects) {
		case SOUNDEFFECTS_NONE:
			return;

		case SOUNDEFFECTS_SYNTH_ONLY:
			play_synth = true;
			break;

		case SOUNDEFFECTS_SAMPLES_PREFERRED:
			break;

		case SOUNDEFFECTS_SYNTH_AND_SAMPLES:
			if (!play_synth) {
				if (Tools_Random_256() & 0x1)
					play_synth = true;
			}
			break;
	}

	if (play_synth) {
		Audio_PlayEffect(soundID);
	}
	else {
		int volume = 255;
		float pan = 0.0f;

		if (position.x != 0 && position.y != 0) {
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

	tile.x = 0;
	tile.y = 0;
	Audio_PlaySoundAtTile(soundID, tile);
}

void
Audio_PlaySoundCutscene(enum SoundID soundID)
{
	if ((soundID == SOUND_INVALID) ||
			(g_enable_sound_effects == SOUNDEFFECTS_NONE ||
			 g_enable_sound_effects == SOUNDEFFECTS_SYNTH_ONLY))
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

	if ((s->soundId != SOUND_INVALID) &&
			(g_enable_sound_effects == SOUNDEFFECTS_SYNTH_ONLY ||
			 g_enable_sound_effects == SOUNDEFFECTS_SYNTH_AND_SAMPLES)) {
		Audio_PlayEffect(s->soundId);
	}

	if (g_enable_voices) {
		for (int i = 0; i < NUM_SPEECH_PARTS; i++) {
			const enum SampleID sampleID
				= (g_table_languageInfo[g_gameConfig.language].sample_prefix != '\0')
				? g_translatedVoice[voiceID][i] : s->voiceId[i];

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
