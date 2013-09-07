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
#include "../os/common.h"
#include "../os/math.h"

#include "coord.h"

#include "orientation.h"
#include "random_general.h"
#include "../enhancement.h"
#include "../map.h"

/** variable_2484: 0x0003A4F4. */
static const int16 s_xOffsets[8] = {
	0, 256, 256, 256, 0, -256, -256, -256
};

/** variable_2474: 0x0003A504. */
static const int16 s_yOffsets[8] = {
	-256, -256, 0, 256, 256, 256, 0, -256
};

/** variable_23DA: 0x0003A45A. */
static const uint16 s_orientationOffsets[4] = {
	0x40, 0x80, 0x00, 0xC0
};

/** variable_23E2: 0x0003A462. */
static const int32 s_gradients[32] = {
	0x3FFF, 0x28BC, 0x145A, 0x0D8E, 0x0A27, 0x081B, 0x06BD, 0x05C3,
	0x0506, 0x0474, 0x03FE, 0x039D, 0x034B, 0x0306, 0x02CB, 0x0297,
	0x026A, 0x0241, 0x021D, 0x01FC, 0x01DE, 0x01C3, 0x01AB, 0x0194,
	0x017F, 0x016B, 0x0159, 0x0148, 0x0137, 0x0128, 0x011A, 0x010C
};

/** variable_3C4C: 0x0003BCCC. */
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

/** variable_3D4C: 0x0003BDCC. */
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
 * @brief   f__07C4_001A_0045_DCB4.
 * @details Likely to have been hand-written assembly.
 */
static uint8
Tile_GetBearing(int16 x1, int16 y1, int16 x2, int16 y2)
{
	static const uint8 orient256[16] = {
		0x20, 0x40, 0x20, 0x00, /* NE, E, NE, N. */
		0xE0, 0xC0, 0xE0, 0x00, /* NW, W, NW, N. */
		0x60, 0x40, 0x60, 0x80, /* SE, E, SE, S. */
		0xA0, 0xC0, 0xA0, 0x80  /* SW, W, SW, S. */
	};

	uint16 index = 0;

	int16 dy = y1 - y2;
	if (dy < 0) {
		index |= 0x8;
		dy = -dy;
	}

	int16 dx = x2 - x1;
	if (dx < 0) {
		index |= 0x4;
		dx = -dx;
	}

	if (dx >= dy) {
		if (((dx + 1) / 2) > dy)
			index |= 0x1;
	}
	else {
		index |= 0x2;
		if (((dy + 1) / 2) > dx)
			index |= 0x1;
	}

	return orient256[index];
}

/**
 * @brief   f__0F3F_0168_0010_C9EF.
 * @details Exact.
 */
uint8
Tile_GetDirectionPacked(uint16 packed_from, uint16 packed_to)
{
	return Tile_GetBearing(
			Tile_GetPackedX(packed_from), Tile_GetPackedY(packed_from),
			Tile_GetPackedX(packed_to),   Tile_GetPackedY(packed_to));
}

/**
 * @brief   f__B4CD_1C1A_001A_9C1B.
 * @details Simplified logic.
 */
uint16
Tile_GetTileInDirectionOf(uint16 packed_from, uint16 packed_to)
{
	if (packed_from == 0 || packed_to == 0)
		return 0;

	uint16 distance = Tile_GetDistancePacked(packed_from, packed_to);
	uint8 orient256 = Tile_GetDirectionPacked(packed_to, packed_from);

	if (distance > 10) {
		distance = min(distance, 20) * 256;

		while (true) {
			int16 dir = 31 + (Tools_Random_256() & 0x3F);
			if (Tools_Random_256() & 1)
				dir = -dir;

			const tile32 tile
				= Tile_MoveByDirection(Tile_UnpackTile(packed_to),
						orient256 + dir, distance);

			const uint16 packed = Tile_PackTile(tile);
			if (Map_IsValidPosition(packed))
				return packed;
		}
	}

	return 0;
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

/**
 * @brief   f__0F3F_00F3_0011_5E67.
 * @details Exact.
 */
tile32
Tile_AddTileDiff(tile32 a, tile32 b)
{
	b.x += a.x;
	b.y += a.y;

	return b;
}

/**
 * @brief   f__0F3F_028E_0015_1153.
 * @details Refactored into bounded and unbounded displacements.
 */
tile32
Tile_MoveByDirection(tile32 tile, uint8 orient256, uint16 distance)
{
	return Tile_MoveByDirectionUnbounded(tile, orient256, min(distance, 0xFF));
}

/**
 * @brief   Displaces tile by the given distance and orientation, unbounded.
 * @details f__0F3F_028E_0015_1153 between labels (l__02A6, l__031C).
 */
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

/**
 * @brief   f__0F3F_01A1_0018_9631.
 * @details Simplified logic.
 */
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

/**
 * @brief   f__B4CD_00A5_0016_24FA.
 * @details Simplified logic.
 */
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
 * @brief   f__B4C1_0000_0022_1807.
 * @details Simplified logic.
 */
uint8
Tile_GetDirection(tile32 from, tile32 to)
{
	int quadrant = 0;   /* SE, SW, NE, NW. */
	int16 dx = to.x - from.x;
	int16 dy = to.y - from.y;
	int32 gradient;
	unsigned int i;

	if (abs(dx) + abs(dy) > 8000) {
		dx /= 2;
		dy /= 2;
	}

	if (dy <= 0) {
		quadrant |= 0x2;
		dy = -dy;
	}

	if (dx < 0) {
		quadrant |= 0x1;
		dx = -dx;
	}

	if (dx >= dy) {
		gradient = (dy != 0) ? ((dx << 8) / dy) : 0x7FFF;
	}
	else {
		gradient = (dx != 0) ? ((dy << 8) / dx) : 0x7FFF;
	}

	for (i = 0; i < lengthof(s_gradients); i++) {
		if (s_gradients[i] <= gradient)
			break;
	}

	if (dx >= dy)
		i = 64 - i;

	if (quadrant == 0 || quadrant == 3) {
		return s_orientationOffsets[quadrant] + 64 - i;
	}
	else {
		return s_orientationOffsets[quadrant] + i;
	}
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
