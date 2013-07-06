/** @file src/tools/coord.h */

#ifndef TOOLS_COORD_H
#define TOOLS_COORD_H

#include "types.h"

extern bool   Tile_IsOutOfMap(uint16 packed);
extern uint8  Tile_GetPackedX(uint16 packed);
extern uint8  Tile_GetPackedY(uint16 packed);
extern uint16 Tile_PackXY(uint16 x, uint16 y);

#endif
