#ifndef TABLE_HOUSEANIMATION_H
#define TABLE_HOUSEANIMATION_H

#include "types.h"
#include "../cutscene.h"

/**
 * The information for a single animation frame in House Animation. It is part
 *  of an array that stops when duration is 0.
 */
typedef struct HouseAnimation_Animation {
	const char *string;     /*!< Name of the WSA for this animation. */
	uint8  duration;        /*!< Duration of this animation. */
	uint8  frameCount;      /*!< Amount of frames in this animation. */
	uint16 flags;           /*!< Flags of the animation. */
} HouseAnimation_Animation;

/**
 * Subtitle information part of House Information. It is part of an array that
 *  stops when stringID is 0xFFFF.
 */
typedef struct HouseAnimation_Subtitle {
	uint16 stringID;        /*!< StringID for the subtitle. */
	uint16 colour;          /*!< Colour of the subtitle. */
	uint8  animationID;     /*!< To which AnimationID this Subtitle belongs. */
	uint8  top;             /*!< The top of the subtitle, in pixels. */
	uint8  waitFadein;      /*!< How long to wait before we fadein this Subtitle. */
	uint8  paletteFadein;   /*!< How many ticks the palette update should take when appearing. */
	uint8  waitFadeout;     /*!< How long to wait before we fadeout this Subtitle. */
	uint8  paletteFadeout;  /*!< How many ticks the palette update should take when disappearing. */
} HouseAnimation_Subtitle;

/**
 * Voice information part of House Information. It is part of an array that
 *  stops when voiceID is 0xFF.
 */
typedef struct HouseAnimation_SoundEffect {
	uint8  animationID;     /*!< The which AnimationID this SoundEffect belongs. */
	uint8  voiceID;         /*!< The SoundEffect to play. */
	uint8  wait;            /*!< How long to wait before we play this SoundEffect. */
} HouseAnimation_SoundEffect;

extern const HouseAnimation_Animation g_table_houseAnimation_animation[HOUSEANIMATION_MAX][32];
extern const HouseAnimation_Subtitle g_table_houseAnimation_subtitle[HOUSEANIMATION_MAX][32];
extern const HouseAnimation_SoundEffect g_table_houseAnimation_soundEffect[HOUSEANIMATION_MAX][90];

#endif
