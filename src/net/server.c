/* server.c */

#include <assert.h>
#include <stdio.h>
#include "enum_string.h"

#include "server.h"

#include "message.h"
#include "../audio/audio.h"
#include "../gui/gui.h"
#include "../house.h"
#include "../string.h"

#if 0
#define SERVER_LOG(FORMAT,...)	\
	do { fprintf(stderr, "%s:%d " FORMAT "\n", __FUNCTION__, __LINE__, __VA_ARGS__); } while (false)
#else
#define SERVER_LOG(...)
#endif

/*--------------------------------------------------------------*/

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

/*--------------------------------------------------------------*/

void
Server_ProcessMessages(void)
{
	const unsigned char *buf = g_client2server_message_buf;
	int count = g_client2server_message_len;

	while (count > 0) {
		const enum ClientServerMsg msg = Net_Decode_ClientServerMsg(buf[0]);
		const int len = Net_GetLength_ClientServerMsg(msg);

		buf++;
		count--;

		if ((msg >= CSMSG_MAX) || (count < len)) {
			SERVER_LOG("msg=%d, len=%d", msg, len);
			break;
		}

		switch (msg) {
			case CSMSG_DISCONNECT:
				assert(false);
				break;

			case CSMSG_MAX:
			case CSMSG_INVALID:
				assert(false);
				break;
		}

		buf += len;
		count -= len;
	}

	g_client2server_message_len = 0;
}
