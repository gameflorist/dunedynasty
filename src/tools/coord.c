/**
 * @file src/tools/coord.c
 *
 * Packed tile and tile32 routines.
 *
 * A packed tile is a uint16 between [0, MAP_SIZE_MAX * MAP_SIZE_MAX),
 * storing the tile position on a map: (y << 6) | x.
 */

#include "coord.h"

/**
 * @brief   Test if the packed tile is invalid.
 * @details Simplification of
 *          (Tile_GetPackedX(packed) > MAP_SIZE_MAX - 1) ||
 *          (Tile_GetPackedY(packed) > MAP_SIZE_MAX - 1).
 */
bool
Tile_IsOutOfMap(uint16 packed)
{
	return (packed & 0xF000);
}

/**
 * @brief   f__0F3F_0322_0011_5AAA.
 * @details Exact.
 */
uint8
Tile_GetPackedX(uint16 packed)
{
	return (packed & 0x3F);
}

/**
 * @brief   f__0F3F_0335_0015_2275.
 * @details Exact.
 */
uint8
Tile_GetPackedY(uint16 packed)
{
	return (packed >> 6) & 0x3F;
}

/**
 * @brief   f__0F3F_034C_0012_18EA.
 * @details Exact.
 */
uint16
Tile_PackXY(uint16 x, uint16 y)
{
	return (y << 6) | x;
}
