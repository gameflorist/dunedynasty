#ifndef AUDIO_AUDIO_H
#define AUDIO_AUDIO_H

#include "../house.h"
#include "../table/sound.h"

extern bool g_enable_audio;
extern bool g_enable_music;
extern bool g_enable_effects;
extern bool g_enable_sounds;
extern bool g_enable_voices;
extern bool g_enable_subtitles;

extern float music_volume;
extern float sound_volume;
extern float voice_volume;

extern void Audio_DisplayMusicName(void);
extern void Audio_GlobMusicInfo(MusicInfo *m, MusicInfoGlob glob[NUM_MUSIC_SETS]);
extern void Audio_ScanMusic(void);
extern void Audio_PlayMusic(enum MusicID musicID);
extern void Audio_PlayMusicIfSilent(enum MusicID musicID);
extern void Audio_AdjustMusicVolume(bool increase, bool adjust_current_track_only);
extern void Audio_PlayEffect(enum SoundID effectID);
extern void Audio_LoadSampleSet(enum HouseType houseID);
extern void Audio_PlaySample(enum SampleID sampleID, int volume, float pan);
extern void Audio_PlaySoundAtTile(enum SoundID soundID, tile32 position);
extern void Audio_PlaySound(enum SoundID soundID);
extern void Audio_PlaySoundCutscene(enum SoundID soundID);
extern void Audio_PlayVoice(enum VoiceID voiceID);
extern bool Audio_Poll(void);

#include "audio_a5.h"

#define Audio_PollMusic         AudioA5_PollMusic
#define Audio_MusicIsPlaying    AudioA5_MusicIsPlaying

#endif
