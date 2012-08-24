#ifndef AUDIO_AUDIOA5_H
#define AUDIO_AUDIOA5_H

#include "types.h"
#include "../table/sound.h"

#if __cplusplus
extern "C" {
#endif

extern void AudioA5_Init(void);
extern void AudioA5_Uninit(void);

extern void AudioA5_InitMusic(const char *filename, int track);
extern void AudioA5_StopMusic(void);
extern void AudioA5_PollMusic(void);
extern bool AudioA5_MusicIsPlaying(void);
extern void AudioA5_PlayEffect(enum SoundID effectID);

extern void AudioA5_StoreSample(enum SampleID sampleID, uint8 file_index, uint32 file_size);
extern bool AudioA5_PlaySample(enum SampleID sampleID, float volume, float pan);
extern bool AudioA5_PollNarrator(void);

#if __cplusplus
}
#endif

#endif
