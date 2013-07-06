/**
 * @file src/tools/coord.c
 *
 * Packed tile and tile32 routines.
 */

#include "coord.h"

bool
Tile_IsOutOfMap(uint16 packed)
{
	return (packed & 0xF000);
}

uint8
Tile_GetPackedX(uint16 packed)
{
	return (packed & 0x3F);
}

uint8
Tile_GetPackedY(uint16 packed)
{
	return (packed >> 6) & 0x3F;
}

uint16
Tile_PackXY(uint16 x, uint16 y)
{
	return (y << 6) | x;
}
