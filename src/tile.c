/** @file src/tile.c %Tile routines. */

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "os/math.h"
#include "os/common.h"

#include "tile.h"

#include "house.h"
#include "map.h"
#include "tools/coord.h"
#include "tools/orientation.h"
#include "tools/random_general.h"

/**
 * Calculates the distance between the two given tiles.
 *
 * @param from The origin.
 * @param to The destination.
 * @return The longest distance between the X or Y coordinates, plus half the shortest.
 */
uint16 Tile_GetDistance(tile32 from, tile32 to)
{
	uint16 distance_x = abs(from.x - to.x);
	uint16 distance_y = abs(from.y - to.y);

	if (distance_x > distance_y) return distance_x + (distance_y / 2);
	return distance_y + (distance_x / 2);
}

/**
 * Adds two tiles together.
 *
 * @param from The origin.
 * @param diff The difference.
 * @return The new coordinates.
 */
tile32 Tile_AddTileDiff(tile32 from, tile32 diff)
{
	tile32 result;

	result.x = from.x + diff.x;
	result.y = from.y + diff.y;

	return result;
}

/**
 * Calculates the distance between the two given packed tiles.
 *
 * @param packed_from The origin.
 * @param packed_to The destination.
 * @return The longest distance between the X or Y coordinates, plus half the shortest.
 */
uint16 Tile_GetDistancePacked(uint16 packed_from, uint16 packed_to)
{
	tile32 from = Tile_UnpackTile(packed_from);
	tile32 to = Tile_UnpackTile(packed_to);

	return Tile_GetDistance(from, to) >> 8;
}

/**
 * Calculates the rounded up distance between the two given packed tiles.
 *
 * @param from The origin.
 * @param to The destination.
 * @return The longest distance between the X or Y coordinates, plus half the shortest.
 */
uint16 Tile_GetDistanceRoundedUp(tile32 from, tile32 to)
{
	return (Tile_GetDistance(from, to) + 0x80) >> 8;
}

/**
 * Remove fog in the radius around the given tile.
 *
 * @param tile The tile to remove fog around.
 * @param radius The radius to remove fog around.
 */
void
Tile_RefreshFogInRadius(tile32 tile, uint16 radius, bool unveil)
{
	uint16 packed;
	uint16 x, y;
	int16 i, j;

	packed = Tile_PackTile(tile);

	if (!Map_IsValidPosition(packed)) return;

	x = Tile_GetPackedX(packed);
	y = Tile_GetPackedY(packed);
	tile = Tile_MakeXY(x, y);

	for (i = -radius; i <= radius; i++) {
		for (j = -radius; j <= radius; j++) {
			tile32 t;

			if ((x + i) < 0 || (x + i) >= 64) continue;
			if ((y + j) < 0 || (y + j) >= 64) continue;

			packed = Tile_PackXY(x + i, y + j);
			t = Tile_MakeXY(x + i, y + j);

			if (Tile_GetDistanceRoundedUp(tile, t) > radius) continue;

			if (unveil) {
				Map_UnveilTile(packed, g_playerHouseID);
			}
			else {
				Map_RefreshTile(packed);
			}
		}
	}
}

void
Tile_RemoveFogInRadius(tile32 tile, uint16 radius)
{
	Tile_RefreshFogInRadius(tile, radius, true);
}

/**
 * Get a tile in the direction of a destination, randomized a bit.
 *
 * @param packed_from The origin.
 * @param packed_to The destination.
 * @return A packed tile.
 */
uint16 Tile_GetTileInDirectionOf(uint16 packed_from, uint16 packed_to)
{
	int16 distance;
	uint8 direction;
	uint8 i;

	if (packed_from == 0 || packed_to == 0) return 0;

	distance = Tile_GetDistancePacked(packed_from, packed_to);
	direction = Tile_GetDirectionPacked(packed_to, packed_from);

	if (distance <= 10) return 0;

	for (i = 0; i < 4; i++) {
		int16 dir;
		tile32 position;
		uint16 packed;

		dir = 29 + (Tools_Random_256() & 0x3F);

		if ((Tools_Random_256() & 1) != 0) dir = -dir;

		position = Tile_UnpackTile(packed_to);
		position = Tile_MoveByDirection(position, direction + dir, min(distance, 20) << 8);
		packed = Tile_PackTile(position);

		if (Map_IsValidPosition(packed)) return packed;
	}

	return 0;
}

/**
 * Get to direction to follow to go from packed_from to packed_to.
 *
 * @param packed_from The origin.
 * @param packed_to The destination.
 * @return The direction.
 */
uint8 Tile_GetDirectionPacked(uint16 packed_from, uint16 packed_to)
{
	static uint8 returnValues[16] = {0x20, 0x40, 0x20, 0x00, 0xE0, 0xC0, 0xE0, 0x00, 0x60, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xA0, 0x80};

	int16 x1, y1, x2, y2;
	int16 dx, dy;
	uint16 index;

	x1 = Tile_GetPackedX(packed_from);
	y1 = Tile_GetPackedY(packed_from);
	x2 = Tile_GetPackedX(packed_to);
	y2 = Tile_GetPackedY(packed_to);

	index = 0;

	dy = y1 - y2;
	if (dy < 0) {
		index |= 0x8;
		dy = -dy;
	}

	dx = x2 - x1;
	if (dx < 0) {
		index |= 0x4;
		dx = -dx;
	}

	if (dx >= dy) {
		if (((dx + 1) / 2) > dy) index |= 0x1;
	} else {
		index |= 0x2;
		if (((dy + 1) / 2) > dx) index |= 0x1;
	}

	return returnValues[index];
}

static const int8 _stepX[256] = {
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

static const int8 _stepY[256] = {
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
 * Get the tile from given tile at given distance in given direction.
 *
 * @param tile The origin.
 * @param orientation The direction to follow.
 * @param distance The distance.
 * @return The tile.
 */
tile32 Tile_MoveByDirection(tile32 tile, int16 orientation, uint16 distance)
{
	distance = min(distance, 0xFF);

	return Tile_MoveByDirectionUnlimited(tile, orientation, distance);
}

tile32
Tile_MoveByDirectionUnlimited(tile32 tile, int16 orientation, uint16 distance)
{
	int diffX, diffY;
	int roundingOffsetX, roundingOffsetY;

	if (distance == 0) return tile;

	diffX = _stepX[orientation & 0xFF];
	diffY = _stepY[orientation & 0xFF];

	/* Always round away from zero */
	roundingOffsetX = diffX < 0 ? -64 : 64;
	roundingOffsetY = diffY < 0 ? -64 : 64;

	tile.x += (diffX * distance + roundingOffsetX) / 128;
	tile.y -= (diffY * distance + roundingOffsetY) / 128;

	return tile;
}

/**
 * Get the tile from given tile at given maximum distance in random direction.
 *
 * @param tile The origin.
 * @param distance The distance maximum.
 * @param center Wether to center the offset of the tile.
 * @return The tile.
 */
tile32 Tile_MoveByRandom(tile32 tile, uint16 distance, bool center)
{
	uint16 x;
	uint16 y;
	tile32 ret;
	uint8 orientation;
	uint16 newDistance;

	if (distance == 0) return tile;

	x = tile.x;
	y = tile.y;

	newDistance = Tools_Random_256();
	while (newDistance > distance) newDistance /= 2;
	distance = newDistance;

	orientation = Tools_Random_256();
	x += ((_stepX[orientation] * distance) / 128) * 16;
	y -= ((_stepY[orientation] * distance) / 128) * 16;

	if (x > 16384 || y > 16384) return tile;

	ret.x = x;
	ret.y = y;

	return center ? Tile_Center(ret) : ret;
}

/**
 * Get to direction to follow to go from \a from to \a to.
 *
 * @param from The origin.
 * @param to The destination.
 * @return The direction.
 */
int8 Tile_GetDirection(tile32 from, tile32 to)
{
	static const uint16 mapOffsets[] = {0x40, 0x80, 0x0, 0xC0};
	static const int32 directions[] = {
			0x3FFF, 0x28BC, 0x145A, 0xD8E,  0xA27, 0x81B, 0x6BD, 0x5C3,  0x506, 0x474, 0x3FE, 0x39D,  0x34B, 0x306, 0x2CB, 0x297,
			0x26A,  0x241,  0x21D,  0x1FC,  0x1DE, 0x1C3, 0x1AB, 0x194,  0x17F, 0x16B, 0x159, 0x148,  0x137, 0x128, 0x11A, 0x10C
	};

	int32 dx;
	int32 dy;
	uint16 loc02;
	int32 loc06;
	uint16 loc08;
	bool invert;
	uint16 loc0C = 0;

	dx = to.x - from.x;
	dy = to.y - from.y;

	if (abs(dx) + abs(dy) > 8000) {
		dx /= 2;
		dy /= 2;
	}

	if (dy <= 0) {
		loc0C |= 2;
		dy = -dy;
	}

	if (dx < 0) {
		loc0C |= 1;
		dx = -dx;
	}

	loc08 = mapOffsets[loc0C];
	invert = false;
	loc06 = 0x7FFF;

	if (dx >= dy) {
		if (dy != 0) loc06 = (dx << 8) / dy;
	} else {
		invert = true;
		if (dx != 0) loc06 = (dy << 8) / dx;
	}

	for (loc02 = 0; loc02 < lengthof(directions); loc02++) {
		if (directions[loc02] <= loc06) break;
	}

	if (!invert) loc02 = 64 - loc02;

	if (loc0C == 0 || loc0C == 3) return (loc08 + 64 - loc02) & 0xFF;

	return (loc08 + loc02) & 0xFF;
}

/**
 * Move to the given orientation looking from the current position.
 * @note returns input position when going out-of-bounds.
 * @param position The position to move from.
 * @param orientation The orientation to move in.
 * @return The new position, or the old in case of out-of-bounds.
 */
tile32 Tile_MoveByOrientation(tile32 position, uint8 orientation)
{
	static const uint16 xOffsets[8] = {0, 256, 256, 256, 0, -256, -256, -256};
	static const uint16 yOffsets[8] = {-256, -256, 0, 256, 256, 256, 0, -256};

	uint16 x;
	uint16 y;

	x = position.x;
	y = position.y;

	orientation = Orientation_256To8(orientation);

	x += xOffsets[orientation];
	y += yOffsets[orientation];

	if (x > 16384 || y > 16384) return position;

	position.x = x;
	position.y = y;

	return position;
}
