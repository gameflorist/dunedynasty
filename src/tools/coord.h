/** @file src/tools/coord.h */

#ifndef TOOLS_COORD_H
#define TOOLS_COORD_H

#include "types.h"

extern bool   Tile_IsOutOfMap(uint16 packed);
extern uint8  Tile_GetPackedX(uint16 packed);
extern uint8  Tile_GetPackedY(uint16 packed);
extern uint16 Tile_PackXY(uint16 x, uint16 y);
extern uint16 Tile_GetDistancePacked(uint16 a, uint16 b);

extern bool   Tile_IsValid(tile32 tile);
extern uint8  Tile_GetPosX(tile32 tile);
extern uint8  Tile_GetPosY(tile32 tile);
extern tile32 Tile_MakeXY(uint16 x, uint16 y);
extern tile32 Tile_Center(tile32 tile);
extern int16  Tile_GetDistance(tile32 a, tile32 b);
extern uint16 Tile_GetDistanceRoundedUp(tile32 a, tile32 b);

extern uint16 Tile_PackTile(tile32 tile);
extern tile32 Tile_UnpackTile(uint16 packed);

#endif
