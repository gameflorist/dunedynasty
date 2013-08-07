#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "enumeration.h"
#include "types.h"
#include "../table/sound.h"

extern void Server_Send_StatusMessage1(enum HouseFlag houses, uint8 priority, uint16 str1);
extern void Server_Send_StatusMessage2(enum HouseFlag houses, uint8 priority, uint16 str1, uint16 str2);
extern void Server_Send_StatusMessage3(enum HouseFlag houses, uint8 priority, uint16 str1, uint16 str2, uint16 str3);
extern void Server_Send_PlaySound(enum HouseFlag houses, enum SoundID soundID);
extern void Server_Send_PlaySoundAtTile(enum HouseFlag houses, enum SoundID soundID, tile32 position);
extern void Server_Send_PlayVoice(enum HouseFlag houses, enum VoiceID voiceID);

extern void Server_ProcessMessages(void);

#endif
