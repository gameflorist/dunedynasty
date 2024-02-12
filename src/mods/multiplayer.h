#ifndef MODS_MULTIPLAYER_H
#define MODS_MULTIPLAYER_H

#include "enumeration.h"
#include "landscape.h"
#include "mapgenerator.h"

enum MultiplayerHouseState {
	MP_HOUSE_UNUSED,
	MP_HOUSE_IN_LOBBY,
	MP_HOUSE_PLAYING,
	MP_HOUSE_WON,
	MP_HOUSE_LOST
};

typedef struct Multiplayer {
	int client[HOUSE_MAX];

	/* Initial credits. */
	uint16 credits;

	/* Current map seed, used in the game. */
	uint32 curr_seed;

	/* Next (working) map seed to play. */
	uint32 next_seed;

	/* Seed to test map generation. */
	uint32 test_seed;

	/* Map seed modes include random, user-defined and hidden from user. */
	uint32 seed_mode;

	/* Starting army can be small or large. */
	enum MapStartingArmy starting_army;

	/* Win condition can be structures or units. */
	enum MapLoseCondition lose_condition;

	LandscapeGeneratorParams landscape_params;

	enum MultiplayerHouseState state[HOUSE_MAX];

	PlayerConfig player_config[HOUSE_MAX];

	enum MapWormCount worm_count;
} Multiplayer;

struct SkirmishData;

extern Multiplayer g_multiplayer;

extern void Multiplayer_Init(void);
extern bool Multiplayer_IsHouseAvailable(enum HouseType houseID);
extern bool Multiplayer_GenHouses(struct SkirmishData *sd);
extern enum MapGeneratorMode Multiplayer_GenerateMap(enum MapGeneratorMode mode);
extern bool Multiplayer_IsAnyHouseLeftPlaying(void);

#endif
