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

extern enum MapGeneratorMode MapGenerator_TransitionState(enum MapGeneratorMode mode, bool success);
extern uint32 MapGenerator_PickRandomSeed(void);

extern void MapGenerator_SaveWorldState(void);
extern void MapGenerator_LoadWorldState(void);

#endif
