/** @file src/tile.c %Tile routines. */

#include "tile.h"

#include "house.h"
#include "map.h"
#include "tools/coord.h"

static void
Map_UnveilTileForHouses(enum HouseFlag houses, enum TileUnveilCause cause,
		uint16 packed)
{
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (houses & (1 << h))
			Map_UnveilTile(h, cause, packed);
	}
}

void
Tile_RefreshFogInRadius(enum HouseFlag houses, enum TileUnveilCause cause,
		tile32 tile, uint16 radius, bool unveil)
{
	uint16 packed = Tile_PackTile(tile);
	if (!Map_IsValidPosition(packed))
		return;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (!House_IsHuman(h))
			houses &= ~(1 << h);
	}

	if (houses == 0)
		return;

	const int x = Tile_GetPackedX(packed);
	const int y = Tile_GetPackedY(packed);
	tile = Tile_MakeXY(x, y);

	for (int i = -radius; i <= radius; i++) {
		if (!(0 <= (x + i) && (x + i) < MAP_SIZE_MAX))
			continue;

		for (int j = -radius; j <= radius; j++) {
			if (!(0 <= (y + j) && (y + j) < MAP_SIZE_MAX))
				continue;

			const tile32 t = Tile_MakeXY(x + i, y + j);
			if (Tile_GetDistanceRoundedUp(tile, t) > radius)
				continue;

			packed = Tile_PackXY(x + i, y + j);
			if (unveil) {
				Map_UnveilTileForHouses(houses, cause, packed);
			}
			else if (houses & (1 << g_playerHouseID)) {
				Map_Client_RefreshTile(cause, packed);
			}
		}
	}
}

void
Tile_RemoveFogInRadius(enum HouseFlag houses, enum TileUnveilCause cause,
		tile32 tile, uint16 radius)
{
	Tile_RefreshFogInRadius(houses, cause, tile, radius, true);
}
