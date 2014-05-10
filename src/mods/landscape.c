/**
 * @file src/mods/landscape.c
 *
 * Dune II landscape generator.  Creates 64x64 landscapes.
 */

#include <assert.h>
#include <string.h>
#include "../os/common.h"
#include "../os/math.h"

#include "landscape.h"

#include "../map.h"
#include "../sprites.h"
#include "../tools/coord.h"
#include "../tools/random_general.h"

static const int8 k_around[21] = {
	  0,
	 -1,  1, -16, 16,
	-17, 17, -15, 15,
	 -2,  2, -32, 32,
	 -4,  4, -64, 64,
	-30, 30, -34, 34
};

static const uint16 k_offsetTable[2][21][4] = {
	{
		{0, 0, 4, 0}, {4, 0, 4, 4}, {0, 0, 0, 4}, {0, 4, 4, 4},
		{0, 0, 0, 2}, {0, 2, 0, 4}, {0, 0, 2, 0}, {2, 0, 4, 0},
		{4, 0, 4, 2}, {4, 2, 4, 4}, {0, 4, 2, 4}, {2, 4, 4, 4},
		{0, 0, 4, 4}, {2, 0, 2, 2}, {0, 0, 2, 2}, {4, 0, 2, 2},
		{0, 2, 2, 2}, {2, 2, 4, 2}, {2, 2, 0, 4}, {2, 2, 4, 4},
		{2, 2, 2, 4},
	},
	{
		{0, 0, 4, 0}, {4, 0, 4, 4}, {0, 0, 0, 4}, {0, 4, 4, 4},
		{0, 0, 0, 2}, {0, 2, 0, 4}, {0, 0, 2, 0}, {2, 0, 4, 0},
		{4, 0, 4, 2}, {4, 2, 4, 4}, {0, 4, 2, 4}, {2, 4, 4, 4},
		{4, 0, 0, 4}, {2, 0, 2, 2}, {0, 0, 2, 2}, {4, 0, 2, 2},
		{0, 2, 2, 2}, {2, 2, 4, 2}, {2, 2, 0, 4}, {2, 2, 4, 4},
		{2, 2, 2, 4},
	},
};

static void
LandscapeGenerator_AddSpiceOnTile(uint16 packed, Tile *map)
{
	int i, j;
	Tile *t;
	Tile *t2;

	t = &map[packed];

	switch (t->groundSpriteID) {
		case LST_SPICE:
			t->groundSpriteID = LST_THICK_SPICE;
			/* Fall through. */
		case LST_THICK_SPICE:
			for (j = -1; j <= 1; j++) {
				for (i = -1; i <= 1; i++) {
					const uint16 packed2 = Tile_PackXY(
							Tile_GetPackedX(packed) + i,
							Tile_GetPackedY(packed) + j);

					if (Tile_IsOutOfMap(packed2))
						continue;

					t2 = &map[packed2];

					if (!g_table_landscapeInfo[t2->groundSpriteID].canBecomeSpice) {
						t->groundSpriteID = LST_SPICE;
					}
					else if (t2->groundSpriteID != LST_THICK_SPICE) {
						t2->groundSpriteID = LST_SPICE;
					}
				}
			}
			return;

		default:
			if (g_table_landscapeInfo[t->groundSpriteID].canBecomeSpice)
				t->groundSpriteID = LST_SPICE;
			return;
	}
}

static void
LandscapeGenerator_MakeRoughLandscape(Tile *map)
{
	unsigned int i, j;
	uint8 memory[273];

	for (i = 0; i < 272; i++) {
		memory[i] = Tools_Random_256() & 0xF;
		if (memory[i] > 0xA)
			memory[i] = 0xA;
	}

	memory[272] = 0;

	i = (Tools_Random_256() & 0xF) + 1;
	while (i-- != 0) {
		const int base = Tools_Random_256();

		for (j = 0; j < lengthof(k_around); j++) {
			const int index = clamp(0, base + k_around[j], 272);
			memory[index] = (memory[index] + (Tools_Random_256() & 0xF)) & 0xF;
		}
	}

	i = (Tools_Random_256() & 0x3) + 1;
	while (i-- != 0) {
		const int base = Tools_Random_256();

		for (j = 0; j < lengthof(k_around); j++) {
			const int index = clamp(0, base + k_around[j], 272);
			memory[index] = Tools_Random_256() & 0x3;
		}
	}

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			const uint16 packed = Tile_PackXY(4*i, 4*j);
			map[packed].groundSpriteID = memory[16*j + i];
		}
	}
}

static void
LandscapeGenerator_AverageRoughLandscape(Tile *map)
{
	unsigned int i, j, k;

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			for (k = 0; k < 21; k++) {
				const uint16 *offset = k_offsetTable[(i + 1) % 2][k];
				const uint16 x1 = 4*i + offset[0];
				const uint16 y1 = 4*j + offset[1];
				const uint16 x2 = 4*i + offset[2];
				const uint16 y2 = 4*j + offset[3];
				uint16 packed1;
				uint16 packed2;
				uint16 packed;
				uint16 sprite1;
				uint16 sprite2;

				packed1 = Tile_PackXY(x1, y1);
				packed2 = Tile_PackXY(x2, y2);
				packed = (packed1 + packed2) / 2;

				if (Tile_IsOutOfMap(packed))
					continue;

				packed1 = Tile_PackXY(x1 & 0x3F, y1);
				packed2 = Tile_PackXY(x2 & 0x3F, y2);

				assert(packed1 < 64 * 64);
				sprite1 = map[packed1].groundSpriteID;

				/* ENHANCEMENT -- use groundSpriteID=0 when
				 * out-of-bounds to generate the original maps.
				 */
				sprite2 = (packed2 < 64 * 64)
					? map[packed2].groundSpriteID : 0;

				map[packed].groundSpriteID = (sprite1 + sprite2 + 1) / 2;
			}
		}
	}
}

static void
LandscapeGenerator_Average(Tile *map)
{
	unsigned int i, j;
	uint16 currRow[64];
	uint16 prevRow[64];

	memset(currRow, 0, sizeof(currRow));

	for (j = 0; j < 64; j++) {
		Tile *t = &map[64 * j];
		memcpy(prevRow, currRow, sizeof(prevRow));

		for (i = 0; i < 64; i++) {
			currRow[i] = t[i].groundSpriteID;
		}

		for (i = 0; i < 64; i++) {
			uint16 sum = 0;

			sum += (i == 0  || j == 0)  ? currRow[i] : prevRow[i - 1];
			sum += (           j == 0)  ? currRow[i] : prevRow[i];
			sum += (i == 63 || j == 0)  ? currRow[i] : prevRow[i + 1];
			sum += (i == 0)             ? currRow[i] : currRow[i - 1];
			sum +=                        currRow[i];
			sum += (i == 63)            ? currRow[i] : currRow[i + 1];
			sum += (i == 0  || j == 63) ? currRow[i] : t[i + 63].groundSpriteID;
			sum += (           j == 63) ? currRow[i] : t[i + 64].groundSpriteID;
			sum += (i == 63 || j == 63) ? currRow[i] : t[i + 65].groundSpriteID;

			t[i].groundSpriteID = sum / 9;
		}
	}
}

static void
LandscapeGenerator_DetermineLandscapeTypes(Tile *map)
{
	unsigned int i;
	uint16 spriteID1;
	uint16 spriteID2;

	spriteID1 = Tools_Random_256() & 0xF;
	if (spriteID1 < 0x8)
		spriteID1 = 0x8;
	if (spriteID1 > 0xC)
		spriteID1 = 0xC;

	spriteID2 = (Tools_Random_256() & 0x3) - 1;
	if (spriteID2 > spriteID1 - 3)
		spriteID2 = spriteID1 - 3;

	for (i = 0; i < 64 * 64; i++) {
		const uint16 spriteID = map[i].groundSpriteID;
		const enum LandscapeType lst
			= (spriteID >  spriteID1 + 4) ? LST_ENTIRELY_MOUNTAIN
			: (spriteID >= spriteID1) ? LST_ENTIRELY_ROCK
			: (spriteID <= spriteID2) ? LST_ENTIRELY_DUNE
			: LST_NORMAL_SAND;

		map[i].groundSpriteID = lst;
	}
}

static void
LandscapeGenerator_AddSpice(Tile *map)
{
	unsigned int i, j;

	i = Tools_Random_256() & 0x2F;
	while (i-- != 0) {
		tile32 tile;

		while (true) {
			const uint16 y = Tools_Random_256() & 0x3F;
			const uint16 x = Tools_Random_256() & 0x3F;
			const uint16 packed = Tile_PackXY(x, y);
			const enum LandscapeType lst = map[packed].groundSpriteID;

			if (g_table_landscapeInfo[lst].canBecomeSpice) {
				tile = Tile_UnpackTile(packed);
				break;
			}
		}

		j = Tools_Random_256() & 0x1F;
		while (j-- != 0) {
			while (true) {
				const uint16 dist = Tools_Random_256() & 0x3F;
				const tile32 tile2 = Tile_MoveByRandom(tile, dist, true);
				const uint16 packed = Tile_PackTile(tile2);

				if (!Tile_IsOutOfMap(packed)) {
					LandscapeGenerator_AddSpiceOnTile(packed, map);
					break;
				}
			}
		}
	}
}

static void
LandscapeGenerator_Smooth(Tile *map)
{
	unsigned int i, j;
	uint16 currRow[64];
	uint16 prevRow[64];

	memset(currRow, 0, sizeof(currRow));

	for (j = 0; j < 64; j++) {
		Tile *t = &map[64 * j];

		memcpy(prevRow, currRow, sizeof(prevRow));

		for (i = 0; i < 64; i++) {
			currRow[i] = t[i].groundSpriteID;
		}

		for (i = 0; i < 64; i++) {
			const uint16 curr   = t[i].groundSpriteID;
			const uint16 up     = (j == 0)  ? curr : prevRow[i];
			const uint16 right  = (i == 63) ? curr : currRow[i + 1];
			const uint16 down   = (j == 63) ? curr : t[i + 64].groundSpriteID;
			const uint16 left   = (i == 0)  ? curr : currRow[i - 1];
			uint16 spriteID = 0;

			if (up    == curr) spriteID |= 1;
			if (right == curr) spriteID |= 2;
			if (down  == curr) spriteID |= 4;
			if (left  == curr) spriteID |= 8;

			switch (curr) {
				case LST_NORMAL_SAND:
					spriteID = 0;
					break;
				case LST_ENTIRELY_ROCK:
					if (up    == LST_ENTIRELY_MOUNTAIN) spriteID |= 1;
					if (right == LST_ENTIRELY_MOUNTAIN) spriteID |= 2;
					if (down  == LST_ENTIRELY_MOUNTAIN) spriteID |= 4;
					if (left  == LST_ENTIRELY_MOUNTAIN) spriteID |= 8;
					spriteID++;
					break;
				case LST_ENTIRELY_DUNE:
					spriteID += 17;
					break;
				case LST_ENTIRELY_MOUNTAIN:
					spriteID += 33;
					break;
				case LST_SPICE:
					if (up    == LST_THICK_SPICE) spriteID |= 1;
					if (right == LST_THICK_SPICE) spriteID |= 2;
					if (down  == LST_THICK_SPICE) spriteID |= 4;
					if (left  == LST_THICK_SPICE) spriteID |= 8;
					spriteID += 49;
					break;
				case LST_THICK_SPICE:
					spriteID += 65;
					break;
				default:
					break;
			}

			t[i].groundSpriteID = spriteID;
		}
	}
}

static void
LandscapeGenerator_Finalise(Tile *map)
{
	const uint16 *iconMap = &g_iconMap[g_iconMap[ICM_ICONGROUP_LANDSCAPE]];
	int i;

	for (i = 0; i < 64 * 64; i++) {
		Tile *t = &map[i];

		t->groundSpriteID   = iconMap[t->groundSpriteID];
		t->overlaySpriteID  = 0;
		t->houseID          = HOUSE_HARKONNEN;
		t->isUnveiled_      = false;
		t->hasUnit          = false;
		t->hasStructure     = false;
		t->hasAnimation     = false;
		t->hasExplosion     = false;
		t->index            = 0;
	}

	for (i = 0; i < 64 * 64; i++) {
		g_mapSpriteID[i] = map[i].groundSpriteID;
	}
}

void
Map_CreateLandscape(uint32 seed)
{
	Tile *map = g_map;

	Tools_Random_Seed(seed);

	/* Place random data on a 4x4 grid. */
	LandscapeGenerator_MakeRoughLandscape(map);

	/* Average around the 4x4 grid. */
	LandscapeGenerator_AverageRoughLandscape(map);

	/* Average each tile with its neighbours. */
	LandscapeGenerator_Average(map);

	/* Filter each tile to determine its final type. */
	LandscapeGenerator_DetermineLandscapeTypes(map);

	/* Add some spice. */
	LandscapeGenerator_AddSpice(map);

	/* Make everything smoother and use the right sprite indexes. */
	LandscapeGenerator_Smooth(map);

	/* Finalise the tiles with the real sprites. */
	LandscapeGenerator_Finalise(map);
}
