/** @file src/tile.c %Tile routines. */

#include "tile.h"

#include "house.h"
#include "map.h"
#include "tools/coord.h"

/**
 * Remove fog in the radius around the given tile.
 *
 * @param tile The tile to remove fog around.
 * @param radius The radius to remove fog around.
 */
void
Tile_RefreshFogInRadius(enum HouseFlag houses,
		tile32 tile, uint16 radius, bool unveil)
{
	uint16 packed;
	uint16 x, y;
	int16 i, j;

	if (!(houses & (1 << g_playerHouseID)))
		return;

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
			else if (houses & (1 << g_playerHouseID)) {
				Map_Client_RefreshTile(packed, 2);
			}
		}
	}
}

void
Tile_RemoveFogInRadius(enum HouseFlag houses, tile32 tile, uint16 radius)
{
	Tile_RefreshFogInRadius(houses, tile, radius, true);
}
