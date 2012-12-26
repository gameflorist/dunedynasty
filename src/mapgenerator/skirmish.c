/* skirmish.c */

#include <assert.h>
#include <stdlib.h>

#include "skirmish.h"

#include "../map.h"
#include "../scenario.h"
#include "../tools.h"

bool
Skirmish_GenerateMap(bool newseed)
{
	uint32 seed;

	if (newseed) {
		/* DuneMaps only supports 15 bit maps seeds, so there. */
		seed = rand() & 0x7FFF;
		g_skirmish.seed = seed;
	}
	else {
		seed = g_skirmish.seed;
	}

	Tools_RandomLCG_Seed(seed);
	Map_CreateLandscape(seed);
	return true;
}
