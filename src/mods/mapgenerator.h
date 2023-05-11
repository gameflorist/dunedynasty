#ifndef MODS_MAPGENERATOR_H
#define MODS_MAPGENERATOR_H

#include "types.h"

enum MapGeneratorMode {
	/* Map generation stopped. */
	MAP_GENERATOR_STOP,

	/* Try the test seed.  On failure, stop. */
	MAP_GENERATOR_TRY_TEST_ELSE_STOP,

	/* Try the test seed.  On failure, try a random seed later. */
	MAP_GENERATOR_TRY_TEST_ELSE_RAND,

	/* Try a random seed.  On failure, stop. */
	MAP_GENERATOR_TRY_RAND_ELSE_STOP,

	/* Try a random seed.  On failure, try another random seed later. */
	MAP_GENERATOR_TRY_RAND_ELSE_RAND,

	/* Try the next seed.  Must succeed. */
	MAP_GENERATOR_FINAL,
};

enum MapSeedMode {
	/* A random seed is chosen automatically. User can request a new one. */
	MAP_SEED_MODE_RANDOM = 0,

	/* A user-defined seed is used for the map. */
	MAP_SEED_MODE_FIXED = 1,

	/* The map seed is random and is not revealed to the players. */
	MAP_SEED_MODE_SURPRISE = 2,
};

enum MapLoseCondition {
	MAP_LOSE_CONDITION_STRUCTURES = 0,
	MAP_LOSE_CONDITION_UNITS = 1
};

extern enum MapGeneratorMode MapGenerator_TransitionState(enum MapGeneratorMode mode, bool success);
extern uint32 MapGenerator_PickRandomSeed(void);

extern void MapGenerator_SaveWorldState(void);
extern void MapGenerator_LoadWorldState(void);

#endif
