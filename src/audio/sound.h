/* $Id$ */

/** @file src/sound.h Sound definitions. */

#ifndef SOUND_H
#define SOUND_H

extern void Music_Play(uint16 musicID);
extern void Voice_PlayAtTile(int16 voiceID, tile32 position);
extern void Voice_Play(int16 voiceID);
extern void Voice_LoadVoices(uint16 voiceSet);
extern void Voice_UnloadVoices(void);

extern void Sound_StartSound(uint16 index);
extern void Sound_Output_Feedback(uint16 index);
extern bool Sound_StartSpeech(void);
extern void *Sound_Unknown0823(const char *filename, uint32 *retFileSize);

#endif /* SOUND_H */
