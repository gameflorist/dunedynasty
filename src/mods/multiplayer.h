#ifndef MODS_MULTIPLAYER_H
#define MODS_MULTIPLAYER_H

#include "enumeration.h"
#include "landscape.h"

enum MultiplayerHouseState {
	MP_HOUSE_UNUSED,
	MP_HOUSE_IN_LOBBY,
	MP_HOUSE_PLAYING,
	MP_HOUSE_WON,
	MP_HOUSE_LOST
};

typedef struct Multiplayer {
	int client[HOUSE_MAX];

	uint16 credits;
	uint32 seed;
	LandscapeGeneratorParams landscape_params;

	enum MultiplayerHouseState state[HOUSE_MAX];
} Multiplayer;

struct SkirmishData;

extern Multiplayer g_multiplayer;

extern void Multiplayer_Init(void);
extern bool Multiplayer_IsHouseAvailable(enum HouseType houseID);
extern bool Multiplayer_GenHouses(struct SkirmishData *sd);
extern bool Multiplayer_GenerateMap(bool newseed);

#endif
