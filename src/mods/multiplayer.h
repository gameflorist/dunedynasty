#ifndef MODS_MULTIPLAYER_H
#define MODS_MULTIPLAYER_H

#include "enumeration.h"
#include "types.h"

typedef struct Multiplayer {
	int client[HOUSE_MAX];
	uint32 seed;
} Multiplayer;

struct SkirmishData;

extern Multiplayer g_multiplayer;

extern bool Multiplayer_GenHouses(struct SkirmishData *sd);

extern bool Multiplayer_IsHouseAvailable(enum HouseType houseID);
extern bool Multiplayer_GenerateMap(bool newseed);

#endif
