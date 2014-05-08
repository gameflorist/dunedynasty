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
Map_AddSpiceOnTile(uint16 packed)
{
	Tile *t;

	t = &g_map[packed];

	switch (t->groundSpriteID) {
		case LST_SPICE:
			t->groundSpriteID = LST_THICK_SPICE;
			Map_AddSpiceOnTile(packed);
			return;

		case LST_THICK_SPICE: {
			int8 i;
			int8 j;

			for (j = -1; j <= 1; j++) {
				for (i = -1; i <= 1; i++) {
					Tile *t2;
					uint16 packed2 = Tile_PackXY(Tile_GetPackedX(packed) + i, Tile_GetPackedY(packed) + j);

					if (Tile_IsOutOfMap(packed2))
						continue;

					t2 = &g_map[packed2];

					if (!g_table_landscapeInfo[t2->groundSpriteID].canBecomeSpice) {
						t->groundSpriteID = LST_SPICE;
						continue;
					}

					if (t2->groundSpriteID != LST_THICK_SPICE)
						t2->groundSpriteID = LST_SPICE;
				}
			}
			return;
		}

		default:
			if (g_table_landscapeInfo[t->groundSpriteID].canBecomeSpice)
				t->groundSpriteID = LST_SPICE;
			return;
	}
}

void
Map_CreateLandscape(uint32 seed)
{
	uint16 i;
	uint16 j;
	uint16 k;
	uint8  memory[273];
	uint16 currentRow[64];
	uint16 previousRow[64];
	uint16 spriteID1;
	uint16 spriteID2;
	uint16 *iconMap;

	Tools_Random_Seed(seed);

	/* Place random data on a 4x4 grid. */
	for (i = 0; i < 272; i++) {
		memory[i] = Tools_Random_256() & 0xF;
		if (memory[i] > 0xA)
			memory[i] = 0xA;
	}

	i = (Tools_Random_256() & 0xF) + 1;
	while (i-- != 0) {
		int16 base = Tools_Random_256();

		for (j = 0; j < lengthof(k_around); j++) {
			int16 index = min(max(0, base + k_around[j]), 272);

			memory[index] = (memory[index] + (Tools_Random_256() & 0xF)) & 0xF;
		}
	}

	i = (Tools_Random_256() & 0x3) + 1;
	while (i-- != 0) {
		int16 base = Tools_Random_256();

		for (j = 0; j < lengthof(k_around); j++) {
			int16 index = min(max(0, base + k_around[j]), 272);

			memory[index] = Tools_Random_256() & 0x3;
		}
	}

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			g_map[Tile_PackXY(i * 4, j * 4)].groundSpriteID = memory[j * 16 + i];
		}
	}

	/* Average around the 4x4 grid. */
	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			for (k = 0; k < 21; k++) {
				const uint16 *offsets = k_offsetTable[(i + 1) % 2][k];
				uint16 packed1;
				uint16 packed2;
				uint16 packed;
				uint16 sprite2;

				packed1 = Tile_PackXY(i * 4 + offsets[0], j * 4 + offsets[1]);
				packed2 = Tile_PackXY(i * 4 + offsets[2], j * 4 + offsets[3]);
				packed = (packed1 + packed2) / 2;

				if (Tile_IsOutOfMap(packed))
					continue;

				packed1 = Tile_PackXY((i * 4 + offsets[0]) & 0x3F, j * 4 + offsets[1]);
				packed2 = Tile_PackXY((i * 4 + offsets[2]) & 0x3F, j * 4 + offsets[3]);
				assert(packed1 < 64 * 64);

				/* ENHANCEMENT -- use groundSpriteID=0 when out-of-bounds to generate the original maps. */
				if (packed2 < 64 * 64) {
					sprite2 = g_map[packed2].groundSpriteID;
				} else {
					sprite2 = 0;
				}

				g_map[packed].groundSpriteID = (g_map[packed1].groundSpriteID + sprite2 + 1) / 2;
			}
		}
	}

	memset(currentRow, 0, 128);

	/* Average each tile with its neighbours. */
	for (j = 0; j < 64; j++) {
		Tile *t = &g_map[j * 64];
		memcpy(previousRow, currentRow, 128);

		for (i = 0; i < 64; i++) {
			currentRow[i] = t[i].groundSpriteID;
		}

		for (i = 0; i < 64; i++) {
			uint16 neighbours[9];
			uint16 total = 0;

			neighbours[0] = (i == 0  || j == 0)  ? currentRow[i] : previousRow[i - 1];
			neighbours[1] = (           j == 0)  ? currentRow[i] : previousRow[i];
			neighbours[2] = (i == 63 || j == 0)  ? currentRow[i] : previousRow[i + 1];
			neighbours[3] = (i == 0)             ? currentRow[i] : currentRow[i - 1];
			neighbours[4] =                        currentRow[i];
			neighbours[5] = (i == 63)            ? currentRow[i] : currentRow[i + 1];
			neighbours[6] = (i == 0  || j == 63) ? currentRow[i] : t[i + 63].groundSpriteID;
			neighbours[7] = (           j == 63) ? currentRow[i] : t[i + 64].groundSpriteID;
			neighbours[8] = (i == 63 || j == 63) ? currentRow[i] : t[i + 65].groundSpriteID;

			for (k = 0; k < 9; k++) total += neighbours[k];
			t[i].groundSpriteID = total / 9;
		}
	}

	/* Filter each tile to determine its final type. */
	spriteID1 = Tools_Random_256() & 0xF;
	if (spriteID1 < 0x8)
		spriteID1 = 0x8;
	if (spriteID1 > 0xC)
		spriteID1 = 0xC;

	spriteID2 = (Tools_Random_256() & 0x3) - 1;
	if (spriteID2 > spriteID1 - 3)
		spriteID2 = spriteID1 - 3;

	for (i = 0; i < 4096; i++) {
		uint16 spriteID = g_map[i].groundSpriteID;

		if (spriteID > spriteID1 + 4)
			spriteID = LST_ENTIRELY_MOUNTAIN;
		else if (spriteID >= spriteID1)
			spriteID = LST_ENTIRELY_ROCK;
		else if (spriteID <= spriteID2)
			spriteID = LST_ENTIRELY_DUNE;
		else
			spriteID = LST_NORMAL_SAND;

		g_map[i].groundSpriteID = spriteID;
	}

	/* Add some spice. */
	i = Tools_Random_256() & 0x2F;
	while (i-- != 0) {
		tile32 tile;
		uint16 packed;

		while (true) {
			packed = Tools_Random_256() & 0x3F;
			packed = Tile_PackXY(Tools_Random_256() & 0x3F, packed);

			if (g_table_landscapeInfo[g_map[packed].groundSpriteID].canBecomeSpice)
				break;
		}

		tile = Tile_UnpackTile(packed);

		j = Tools_Random_256() & 0x1F;
		while (j-- != 0) {
			while (true) {
				packed = Tile_PackTile(Tile_MoveByRandom(tile, Tools_Random_256() & 0x3F, true));

				if (!Tile_IsOutOfMap(packed))
					break;
			}

			Map_AddSpiceOnTile(packed);
		}
	}

	/* Make everything smoother and use the right sprite indexes. */
	for (j = 0; j < 64; j++) {
		Tile *t = &g_map[j * 64];

		memcpy(previousRow, currentRow, 128);

		for (i = 0; i < 64; i++) currentRow[i] = t[i].groundSpriteID;

		for (i = 0; i < 64; i++) {
			uint16 current = t[i].groundSpriteID;
			uint16 up      = (j == 0)  ? current : previousRow[i];
			uint16 left    = (i == 63) ? current : currentRow[i + 1];
			uint16 down    = (j == 63) ? current : t[i + 64].groundSpriteID;
			uint16 right   = (i == 0)  ? current : currentRow[i - 1];
			uint16 spriteID = 0;

			if (up    == current) spriteID |= 1;
			if (left  == current) spriteID |= 2;
			if (down  == current) spriteID |= 4;
			if (right == current) spriteID |= 8;

			switch (current) {
				case LST_NORMAL_SAND:
					spriteID = 0;
					break;
				case LST_ENTIRELY_ROCK:
					if (up    == LST_ENTIRELY_MOUNTAIN) spriteID |= 1;
					if (left  == LST_ENTIRELY_MOUNTAIN) spriteID |= 2;
					if (down  == LST_ENTIRELY_MOUNTAIN) spriteID |= 4;
					if (right == LST_ENTIRELY_MOUNTAIN) spriteID |= 8;
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
					if (left  == LST_THICK_SPICE) spriteID |= 2;
					if (down  == LST_THICK_SPICE) spriteID |= 4;
					if (right == LST_THICK_SPICE) spriteID |= 8;
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

	/* Finalise the tiles with the real sprites. */
	iconMap = &g_iconMap[g_iconMap[ICM_ICONGROUP_LANDSCAPE]];

	for (i = 0; i < 4096; i++) {
		Tile *t = &g_map[i];

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

	for (i = 0; i < 4096; i++) {
		g_mapSpriteID[i] = g_map[i].groundSpriteID;
	}
}
