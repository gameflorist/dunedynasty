/**
 * @file src/tools/coord.c
 *
 * Packed tile and tile32 routines.
 *
 * A packed tile is a uint16 between [0, MAP_SIZE_MAX * MAP_SIZE_MAX),
 * storing the tile position on a map: (y << 6) | x.
 *
 * A tile32 is a uint32 storing the "real" position: (y << 16) | x.
 * 16 units of tile32 corresponds to one pixel.  The largest value for
 * x and y is therefore (MAP_SIZE_MAX * 16 * 16 - 1).
 */

#include <stdlib.h>
#include "../os/math.h"

#include "coord.h"

#include "orientation.h"
#include "random_general.h"
#include "../enhancement.h"

static const int16 s_xOffsets[8] = {
	0, 256, 256, 256, 0, -256, -256, -256
};

static const int16 s_yOffsets[8] = {
	-256, -256, 0, 256, 256, 256, 0, -256
};

static const int8 s_stepX[256] = {
	   0,    3,    6,    9,   12,   15,   18,   21,   24,   27,   30,   33,   36,   39,   42,   45,
	  48,   51,   54,   57,   59,   62,   65,   67,   70,   73,   75,   78,   80,   82,   85,   87,
	  89,   91,   94,   96,   98,  100,  101,  103,  105,  107,  108,  110,  111,  113,  114,  116,
	 117,  118,  119,  120,  121,  122,  123,  123,  124,  125,  125,  126,  126,  126,  126,  126,
	 127,  126,  126,  126,  126,  126,  125,  125,  124,  123,  123,  122,  121,  120,  119,  118,
	 117,  116,  114,  113,  112,  110,  108,  107,  105,  103,  102,  100,   98,   96,   94,   91,
	  89,   87,   85,   82,   80,   78,   75,   73,   70,   67,   65,   62,   59,   57,   54,   51,
	  48,   45,   42,   39,   36,   33,   30,   27,   24,   21,   18,   15,   12,    9,    6,    3,
	   0,   -3,   -6,   -9,  -12,  -15,  -18,  -21,  -24,  -27,  -30,  -33,  -36,  -39,  -42,  -45,
	 -48,  -51,  -54,  -57,  -59,  -62,  -65,  -67,  -70,  -73,  -75,  -78,  -80,  -82,  -85,  -87,
	 -89,  -91,  -94,  -96,  -98, -100, -102, -103, -105, -107, -108, -110, -111, -113, -114, -116,
	-117, -118, -119, -120, -121, -122, -123, -123, -124, -125, -125, -126, -126, -126, -126, -126,
	-126, -126, -126, -126, -126, -126, -125, -125, -124, -123, -123, -122, -121, -120, -119, -118,
	-117, -116, -114, -113, -112, -110, -108, -107, -105, -103, -102, -100,  -98,  -96,  -94,  -91,
	 -89,  -87,  -85,  -82,  -80,  -78,  -75,  -73,  -70,  -67,  -65,  -62,  -59,  -57,  -54,  -51,
	 -48,  -45,  -42,  -39,  -36,  -33,  -30,  -27,  -24,  -21,  -18,  -15,  -12,   -9,   -6,   -3
};

static const int8 s_stepY[256] = {
	 127,  126,  126,  126,  126,  126,  125,  125,  124,  123,  123,  122,  121,  120,  119,  118,
	 117,  116,  114,  113,  112,  110,  108,  107,  105,  103,  102,  100,   98,   96,   94,   91,
	  89,   87,   85,   82,   80,   78,   75,   73,   70,   67,   65,   62,   59,   57,   54,   51,
	  48,   45,   42,   39,   36,   33,   30,   27,   24,   21,   18,   15,   12,    9,    6,    3,
	   0,   -3,   -6,   -9,  -12,  -15,  -18,  -21,  -24,  -27,  -30,  -33,  -36,  -39,  -42,  -45,
	 -48,  -51,  -54,  -57,  -59,  -62,  -65,  -67,  -70,  -73,  -75,  -78,  -80,  -82,  -85,  -87,
	 -89,  -91,  -94,  -96,  -98, -100, -102, -103, -105, -107, -108, -110, -111, -113, -114, -116,
	-117, -118, -119, -120, -121, -122, -123, -123, -124, -125, -125, -126, -126, -126, -126, -126,
	-126, -126, -126, -126, -126, -126, -125, -125, -124, -123, -123, -122, -121, -120, -119, -118,
	-117, -116, -114, -113, -112, -110, -108, -107, -105, -103, -102, -100,  -98,  -96,  -94,  -91,
	 -89,  -87,  -85,  -82,  -80,  -78,  -75,  -73,  -70,  -67,  -65,  -62,  -59,  -57,  -54,  -51,
	 -48,  -45,  -42,  -39,  -36,  -33,  -30,  -27,  -24,  -21,  -18,  -15,  -12,   -9,   -6,   -3,
	   0,    3,    6,    9,   12,   15,   18,   21,   24,   27,   30,   33,   36,   39,   42,   45,
	  48,   51,   54,   57,   59,   62,   65,   67,   70,   73,   75,   78,   80,   82,   85,   87,
	  89,   91,   94,   96,   98,  100,  101,  103,  105,  107,  108,  110,  111,  113,  114,  116,
	 117,  118,  119,  120,  121,  122,  123,  123,  124,  125,  125,  126,  126,  126,  126,  126
};

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

/**
 * @brief   f__0F3F_0360_0038_97C0.
 * @details Simplified logic.
 */
uint16
Tile_GetDistancePacked(uint16 a, uint16 b)
{
	const uint16 dx = abs(Tile_GetPackedX(a) - Tile_GetPackedX(b));
	const uint16 dy = abs(Tile_GetPackedY(a) - Tile_GetPackedY(b));

	if (dx > dy) {
		return dx + (dy / 2);
	}
	else {
		return dy + (dx / 2);
	}
}

/**
 * @brief   f__0F3F_000D_0019_5076.
 * @details Exact: return (*(uint32*)(&tile) & 0xC000C000) == 0;
 */
bool
Tile_IsValid(tile32 tile)
{
	return ((tile.x & 0xC000) == 0) && ((tile.y & 0xC000) == 0);
}

/**
 * @brief   f__0F3F_0046_000C_9E1E.
 * @details xchgb(al, ah), xorb(ah, ah) is (ax >> 8).
 */
uint8
Tile_GetPosX(tile32 tile)
{
	return (tile.x >> 8);
}

/**
 * @brief   f__0F3F_0052_000C_9E02.
 * @details xchgb(al, ah), xorb(ah, ah) is (ax >> 8).
 */
uint8
Tile_GetPosY(tile32 tile)
{
	return (tile.y >> 8);
}

/**
 * @brief   f__0F3F_0037_000F_E3D8.
 * @details Since x <= 64, y <= 64, xchgb is (x << 8), (y << 8).
 */
tile32
Tile_MakeXY(uint16 x, uint16 y)
{
	tile32 tile;

	tile.x = (x << 8);
	tile.y = (y << 8);

	return tile;
}

/**
 * @brief   f__0F3F_0275_0019_64C3.
 * @details Simplified logic.
 */
tile32
Tile_Center(tile32 tile)
{
	tile.x = (tile.x & 0xFF00) | 0x80;
	tile.y = (tile.y & 0xFF00) | 0x80;

	return tile;
}

tile32
Tile_AddTileDiff(tile32 a, tile32 b)
{
	b.x += a.x;
	b.y += a.y;

	return b;
}

tile32
Tile_MoveByDirection(tile32 tile, uint8 orient256, uint16 distance)
{
	return Tile_MoveByDirectionUnbounded(tile, orient256, min(distance, 0xFF));
}

tile32
Tile_MoveByDirectionUnbounded(tile32 tile, uint8 orient256, uint16 distance)
{
	if (distance == 0)
		return tile;

	const int8 diffX = s_stepX[orient256];
	const int8 diffY = s_stepY[orient256];

	/** ENHANCEMENT -- rounding lead to asymmetries when moving N/W vs S/E. */
	if (g_dune2_enhanced) {
		const int8 roundingOffsetX = (diffX < 0) ? -64 : 64;
		const int8 roundingOffsetY = (diffY < 0) ? -64 : 64;

		tile.x += (diffX * distance + roundingOffsetX) / 128;
		tile.y -= (diffY * distance + roundingOffsetY) / 128;
	}
	else {
		tile.x += (64 + diffX * distance) / 128;
		tile.y += (64 - diffY * distance) / 128;
	}

	return tile;
}

tile32
Tile_MoveByRandom(tile32 tile, uint16 distance, bool centre)
{
	if (distance == 0)
		return tile;

	uint16 newDistance = Tools_Random_256();
	while (newDistance > distance) {
		newDistance /= 2;
	}
	distance = newDistance;

	const uint8 orient256 = Tools_Random_256();
	const uint16 x = tile.x + ((s_stepX[orient256] * distance) / 128) * 16;
	const uint16 y = tile.y - ((s_stepY[orient256] * distance) / 128) * 16;

	/* Note: 16384 = MAP_SIZE_MAX * 256. */
	if (x > 16384 || y > 16384)
		return tile;

	tile.x = x;
	tile.y = y;

	return centre ? Tile_Center(tile) : tile;
}

tile32
Tile_MoveByOrientation(tile32 tile, uint8 orient256)
{
	const uint8 orient8 = Orientation_256To8(orient256);
	const uint16 x = tile.x + s_xOffsets[orient8];
	const uint16 y = tile.y + s_yOffsets[orient8];

	/* Note: 16384 = MAP_SIZE_MAX * 256. */
	if (x > 16384 || y > 16384)
		return tile;

	tile.x = x;
	tile.y = y;

	return tile;
}

/**
 * @brief   f__0F3F_00B4_002A_89B2.
 * @details Simplified logic.
 */
int16
Tile_GetDistance(tile32 a, tile32 b)
{
	const uint16 dx = abs(a.x - b.x);
	const uint16 dy = abs(a.y - b.y);

	if (dx > dy) {
		return dx + (dy / 2);
	}
	else {
		return dy + (dx / 2);
	}
}

/**
 * @brief   f__0F3F_0104_0013_C3B8.
 * @details Exact.
 */
uint16
Tile_GetDistanceRoundedUp(tile32 a, tile32 b)
{
	return (Tile_GetDistance(a, b) + 0x80) >> 8;
}

/**
 * @brief   f__0F3F_0086_0017_EA43.
 * @details Simplified logic.
 */
uint16
Tile_PackTile(tile32 tile)
{
	return Tile_PackXY(Tile_GetPosX(tile), Tile_GetPosY(tile));
}

/**
 * @brief   f__0F3F_009D_0017_8464.
 * @details Simplified logic.
 */
tile32
Tile_UnpackTile(uint16 packed)
{
	tile32 tile;

	tile.x = (Tile_GetPackedX(packed) << 8) | 0x80;
	tile.y = (Tile_GetPackedY(packed) << 8) | 0x80;

	return tile;
}
