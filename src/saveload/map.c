/* map.c */

#include "saveload.h"
#include "../map.h"
#include "../sprites.h"

/**
 * Load all Tiles from a file.
 * @param fp The file to load from.
 * @param length The length of the data chunk.
 * @return True if and only if all bytes were read successful.
 */
bool Map_Load(FILE *fp, uint32 length)
{
	uint16 i;

	for (i = 0; i < 0x1000; i++) {
		Tile *t = &g_map[i];

		t->isUnveiled = false;
		t->overlaySpriteID = g_veiledSpriteID;
	}

	while (length >= sizeof(uint16) + sizeof(Tile)) {
		Tile *t;

		length -= sizeof(uint16) + sizeof(Tile);

		if (fread(&i, sizeof(uint16), 1, fp) != 1) return false;
		if (i >= 0x1000) return false;

		t = &g_map[i];
		if (fread(t, sizeof(Tile), 1, fp) != 1) return false;

		if (g_mapSpriteID[i] != t->groundSpriteID) {
			g_mapSpriteID[i] |= 0x8000;
		}
	}
	if (length != 0) return false;

	return true;
}

/**
 * Save all Tiles to a file.
 * @param fp The file to save to.
 * @return True if and only if all bytes were written successful.
 */
bool Map_Save(FILE *fp)
{
	uint16 i;

	for (i = 0; i < 0x1000; i++) {
		Tile *tile = &g_map[i];

		/* If there is nothing on the tile, not unveiled, and it is equal to the mapseed generated tile, don't store it */
		if (!tile->isUnveiled && !tile->hasStructure && !tile->hasUnit && !tile->hasAnimation && !tile->hasExplosion && (g_mapSpriteID[i] & 0x8000) == 0 && g_mapSpriteID[i] == tile->groundSpriteID) continue;

		/* Store the index, then the tile itself */
		if (fwrite(&i, sizeof(uint16), 1, fp) != 1) return false;
		if (fwrite(tile, sizeof(Tile), 1, fp) != 1) return false;
	}

	return true;
}
