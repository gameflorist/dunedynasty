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

bool
Tile_IsValid(tile32 tile)
{
	return ((tile.x & 0xC000) == 0) && ((tile.y & 0xC000) == 0);
}

uint8
Tile_GetPosX(tile32 tile)
{
	return (tile.x >> 8);
}

uint8
Tile_GetPosY(tile32 tile)
{
	return (tile.y >> 8);
}

tile32
Tile_MakeXY(uint16 x, uint16 y)
{
	tile32 tile;

	tile.x = (x << 8);
	tile.y = (y << 8);

	return tile;
}

tile32
Tile_Center(tile32 tile)
{
	tile.x = (tile.x & 0xFF00) | 0x80;
	tile.y = (tile.y & 0xFF00) | 0x80;

	return tile;
}

uint16
Tile_PackTile(tile32 tile)
{
	return Tile_PackXY(Tile_GetPosX(tile), Tile_GetPosY(tile));
}

tile32
Tile_UnpackTile(uint16 packed)
{
	tile32 tile;

	tile.x = (Tile_GetPackedX(packed) << 8) | 0x80;
	tile.y = (Tile_GetPackedY(packed) << 8) | 0x80;

	return tile;
}
