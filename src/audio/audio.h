#ifndef AUDIO_AUDIO_H
#define AUDIO_AUDIO_H

#include "../house.h"
#include "../table/sound.h"

extern float music_volume;
extern float sound_volume;
extern float voice_volume;

extern void Audio_PlayMusic(enum MusicID musicID);
extern void Audio_LoadSampleSet(enum HouseType houseID);
extern void Audio_PlaySample(enum SampleID sampleID, int volume, float pan);
extern void Audio_PlaySoundAtTile(enum SoundID soundID, tile32 position);
extern void Audio_PlaySound(enum SoundID soundID);
extern void Audio_PlayVoice(enum VoiceID voiceID);
extern bool Audio_Poll(void);

#include "audio_a5.h"

#define Audio_PollMusic         AudioA5_PollMusic
#define Audio_MusicIsPlaying    AudioA5_MusicIsPlaying

#endif
