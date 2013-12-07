#ifndef MODS_MULTIPLAYER_H
#define MODS_MULTIPLAYER_H

#include "enumeration.h"
#include "types.h"

enum MultiplayerHouseState {
	MP_HOUSE_UNUSED,
	MP_HOUSE_IN_LOBBY,
	MP_HOUSE_PLAYING,
	MP_HOUSE_WON,
	MP_HOUSE_LOST
};

typedef struct Multiplayer {
	int client[HOUSE_MAX];
	uint32 seed;
	enum MultiplayerHouseState state[HOUSE_MAX];
} Multiplayer;

struct SkirmishData;

extern Multiplayer g_multiplayer;

extern bool Multiplayer_GenHouses(struct SkirmishData *sd);

extern bool Multiplayer_IsHouseAvailable(enum HouseType houseID);
extern bool Multiplayer_GenerateMap(bool newseed);

#endif
