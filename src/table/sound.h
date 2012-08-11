#ifndef TABLE_SOUND_H
#define TABLE_SOUND_H

#include "types.h"

/** Number of voices in the game. */
#define NUM_VOICES 131

/** Maximal number of spoken audio fragments in one message. */
#define NUM_SPEECH_PARTS 5

/** Information about sound files. */
typedef struct SoundData {
	const char *string; /*!< Pointer to a string. */
	uint16 variable_04; /*!< ?? */
} SoundData;

/** Audio and visual feedback about events and commands. */
typedef struct Feedback {
	uint16 voiceId[NUM_SPEECH_PARTS]; /*!< English spoken text. */
	uint16 messageId;                 /*!< Message to display in the viewport when audio is disabled. */
	uint16 soundId;                   /*!< Sound. */
} Feedback;

extern const SoundData g_table_voices[];
extern const SoundData g_table_musics[];
extern const uint16 g_table_voiceMapping[];
extern const Feedback g_feedback[];
extern const uint16 g_translatedVoice[][NUM_SPEECH_PARTS];

#endif
