#ifndef AUDIO_AUDIO_H
#define AUDIO_AUDIO_H

#include "../table/sound.h"

enum SoundEffectSources {
	SOUNDEFFECTS_NONE,
	SOUNDEFFECTS_SYNTH_ONLY,
	SOUNDEFFECTS_SAMPLES_PREFERRED,
	SOUNDEFFECTS_SYNTH_AND_SAMPLES
};

enum MidiFormat {
	MIDI_FORMAT_PCS,
	MIDI_FORMAT_TAN,
	MIDI_FORMAT_GM,
	MIDI_FORMAT_MT32,

	NUM_MIDI_FORMATS,
	MIDI_FORMAT_INVALID = 0xFFFF
};

extern bool g_enable_audio;
extern bool g_enable_music;
extern enum SoundEffectSources g_enable_sound_effects;
extern bool g_enable_voices;
extern bool g_enable_subtitles;

extern float music_volume;
extern float sound_volume;
extern float cutscene_sound_volume;
extern bool g_disable_attack_music;

extern bool g_opl_mame;
extern char sound_font_path[1024];
extern int g_midi_device_id;
extern enum MidiFormat g_midi_format;
extern enum MusicSet default_music_pack;
extern char music_message[128];

extern void Audio_DisplayMusicName(void);
extern void Audio_ScanMusic(void);
extern void Audio_PlayMusic(enum MusicID musicID);
extern void Audio_PlayMusicFile(const MusicList *l, MusicInfo *m);
extern void Audio_PlayMusicIfSilent(enum MusicID musicID);
extern void Audio_PlayMusicNextInSequence(void);
extern void Audio_StopMusicUnlessMenu(void);
extern void Audio_AdjustMusicVolume(float delta, bool adjust_current_track_only);
extern void Audio_PlayEffect(enum SoundID effectID);
extern void Audio_LoadSampleSet(enum SampleSet setID);
extern void Audio_PlaySample(enum SampleID sampleID, int volume, float pan);
extern void Audio_PlaySoundAtTile(enum SoundID soundID, tile32 position);
extern void Audio_PlaySound(enum SoundID soundID);
extern void Audio_PlaySoundCutscene(enum SoundID soundID);
extern void Audio_PlayVoice(enum VoiceID voiceID);
extern void Audio_PlayVoiceAtTile(enum VoiceID voiceID, uint16 packed);
extern bool Audio_Poll(void);

#include "audio_a5.h"

#define Audio_PollMusic         AudioA5_PollMusic
#define Audio_MusicIsPlaying    AudioA5_MusicIsPlaying

#endif
