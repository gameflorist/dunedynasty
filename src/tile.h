/** @file src/tile.h %Tile definitions. */

#ifndef TILE_H
#define TILE_H

#include "types.h"

extern void Tile_RefreshFogInRadius(tile32 tile, uint16 radius, bool unveil);
extern void Tile_RemoveFogInRadius(tile32 tile, uint16 radius);

#endif /* TILE_H */
