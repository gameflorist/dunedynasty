/** @file src/mods/landscape.h */

#ifndef MODS_LANDSCAPE_H
#define MODS_LANDSCAPE_H

#include "types.h"

struct Tile;

typedef struct {
	unsigned int min_spice_fields;
	unsigned int max_spice_fields;
} LandscapeGeneratorParams;

extern void Map_CreateLandscape(uint32 seed, const LandscapeGeneratorParams *params, struct Tile *map);

#endif
