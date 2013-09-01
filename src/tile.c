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
#include "tools/random_general.h"

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
				Map_RefreshTile(packed, 2);
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

	if (packed_from == 0 || packed_to == 0) return 0;

	distance = Tile_GetDistancePacked(packed_from, packed_to);
	direction = Tile_GetDirectionPacked(packed_to, packed_from);

	if (distance <= 10) return 0;

	while (true) {
		int16 dir;
		tile32 position;
		uint16 packed;

		dir = 31 + (Tools_Random_256() & 0x3F);

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
	static const uint8 returnValues[16] = {0x20, 0x40, 0x20, 0x00, 0xE0, 0xC0, 0xE0, 0x00, 0x60, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xA0, 0x80};

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

/**
 * Get to direction to follow to go from \a from to \a to.
 *
 * @param from The origin.
 * @param to The destination.
 * @return The direction.
 */
int8 Tile_GetDirection(tile32 from, tile32 to)
{
	static const uint16 orientationOffsets[] = {0x40, 0x80, 0x0, 0xC0};
	static const int32 directions[] = {
			0x3FFF, 0x28BC, 0x145A, 0xD8E,  0xA27, 0x81B, 0x6BD, 0x5C3,  0x506, 0x474, 0x3FE, 0x39D,  0x34B, 0x306, 0x2CB, 0x297,
			0x26A,  0x241,  0x21D,  0x1FC,  0x1DE, 0x1C3, 0x1AB, 0x194,  0x17F, 0x16B, 0x159, 0x148,  0x137, 0x128, 0x11A, 0x10C
	};

	int32 dx;
	int32 dy;
	uint16 i;
	int32 gradient;
	uint16 baseOrientation;
	bool invert;
	uint16 quadrant = 0;

	dx = to.x - from.x;
	dy = to.y - from.y;

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

	baseOrientation = orientationOffsets[quadrant];
	invert = false;
	gradient = 0x7FFF;

	if (dx >= dy) {
		if (dy != 0) gradient = (dx << 8) / dy;
	} else {
		invert = true;
		if (dx != 0) gradient = (dy << 8) / dx;
	}

	for (i = 0; i < lengthof(directions); i++) {
		if (directions[i] <= gradient) break;
	}

	if (!invert) i = 64 - i;

	if (quadrant == 0 || quadrant == 3) return (baseOrientation + 64 - i) & 0xFF;

	return (baseOrientation + i) & 0xFF;
}
