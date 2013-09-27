/* server.c */

#include <assert.h>
#include "enum_string.h"

#include "server.h"

#include "../audio/audio.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../string.h"

void
Server_Send_StatusMessage1(enum HouseFlag houses, uint8 priority,
		uint16 str)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str, STR_NULL, STR_NULL);
	}
}

void
Server_Send_StatusMessage2(enum HouseFlag houses, uint8 priority,
		uint16 str1, uint16 str2)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str1, str2, STR_NULL);
	}
}

void
Server_Send_StatusMessage3(enum HouseFlag houses, uint8 priority,
		uint16 str1, uint16 str2, uint16 str3)
{
	if (houses & (1 << g_playerHouseID)) {
		GUI_DrawStatusBarTextWrapper(priority, str1, str2, str3);
	}
}

void
Server_Send_PlaySound(enum HouseFlag houses, enum SoundID soundID)
{
	if (soundID == SOUND_INVALID)
		return;

	if (houses & (1 << g_playerHouseID)) {
		Audio_PlaySound(soundID);
	}
}

void
Server_Send_PlaySoundAtTile(enum HouseFlag houses,
		enum SoundID soundID, tile32 position)
{
	if (soundID == SOUND_INVALID)
		return;

	if (houses & (1 << g_playerHouseID)) {
		Audio_PlaySoundAtTile(soundID, position);
	}
}

void
Server_Send_PlayVoice(enum HouseFlag houses, enum VoiceID voiceID)
{
	if (voiceID == VOICE_INVALID)
		return;

	if (houses & (1 << g_playerHouseID)) {
		Audio_PlayVoice(voiceID);
	}
}
