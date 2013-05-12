/** @file src/saveload/map.c Load/save routines for Map. */

#include "saveload.h"
#include "../map.h"
#include "../sprites.h"
#include "../timer/timer.h"

/**
 * Load a Tile structure to a file (Little endian)
 *
 * @param t The tile to read
 * @param fp The stream
 * @return True if the tile was loaded successfully
 */
static bool fread_tile(Tile *t, FILE *fp)
{
	uint8 buffer[4];
	if (fread(buffer, 1, 4, fp) != 4) return false;
	t->groundSpriteID = buffer[0] | ((buffer[1] & 1) << 8);
	t->overlaySpriteID = buffer[1] >> 1;
	t->houseID = buffer[2] & 0x07;
	t->isUnveiled = (buffer[2] & 0x08) ? true : false;
	t->hasUnit =  (buffer[2] & 0x10) ? true : false;
	t->hasStructure = (buffer[2] & 0x20) ? true : false;
	t->hasAnimation = (buffer[2] & 0x40) ? true : false;
	t->hasExplosion = (buffer[2] & 0x80) ? true : false;
	t->index = buffer[3];
	return true;
}

/**
 * Save a Tile structure to a file (Little endian)
 *
 * @param t The tile to save
 * @param fp The stream
 * @return True if the tile was saved successfully
 */
static bool fwrite_tile(const Tile *t, FILE *fp)
{
	uint8 buffer[4];
	buffer[0] = t->groundSpriteID & 0xff;
	buffer[1] = (t->groundSpriteID >> 8) | (t->overlaySpriteID << 1);
	buffer[2] = t->houseID | (t->isUnveiled << 3) | (t->hasUnit << 4) | (t->hasStructure << 5) | (t->hasAnimation << 6) | (t->hasExplosion << 7);
	buffer[3] = t->index;
	if (fwrite(buffer, 1, 4, fp) != 4) return false;
	return true;
}

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

		if (!fread_le_uint16(&i, fp)) return false;
		if (i >= 0x1000) return false;

		t = &g_map[i];
		if (!fread_tile(t, fp)) return false;

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
		if (!fwrite_le_uint16(i, fp)) return false;
		if (!fwrite_tile(tile, fp)) return false;
	}

	return true;
}

/*--------------------------------------------------------------*/

/* Default fog of war data if nothing was saved. */
void
Map_Load2Fallback(void)
{
	for (uint16 packed = 0; packed < MAP_SIZE_MAX * MAP_SIZE_MAX; packed++) {
		const Tile *t = &g_map[packed];
		FogOfWarTile *f = &g_mapVisible[packed];

		f->timeout          = 0;
		f->groundSpriteID   = t->groundSpriteID;
		f->overlaySpriteID  = (g_veiledSpriteID - 16 <= t->overlaySpriteID && t->overlaySpriteID <= g_veiledSpriteID) ? 0 : t->overlaySpriteID;
		f->houseID          = t->houseID;
		f->hasStructure     = t->hasStructure;
	}
}

bool
Map_Load2(FILE *fp, uint32 length)
{
	while (length >= 3 * sizeof(uint16) + 2 * sizeof(uint8)) {
		uint16 packed;
		uint16 timeout;
		uint16 spriteID;
		uint8 houseID;
		uint8 hasStructure;

		if (fread(&packed,      sizeof(uint16), 1, fp) != 1) return false;
		if (fread(&timeout,     sizeof(uint16), 1, fp) != 1) return false;
		if (fread(&spriteID,    sizeof(uint16), 1, fp) != 1) return false;
		if (fread(&houseID,     sizeof(uint8),  1, fp) != 1) return false;
		if (fread(&hasStructure,sizeof(uint8),  1, fp) != 1) return false;

		FogOfWarTile *f = &g_mapVisible[packed];
		f->timeout          = (timeout == 0) ? 0 : (g_timerGame + timeout);
		f->groundSpriteID   = (spriteID & 0x1FF);
		f->overlaySpriteID  = (spriteID >> 9);
		f->houseID          = houseID;
		f->hasStructure     = hasStructure;

		length -= 3 * sizeof(uint16) + 2 * sizeof(uint8);
	}

	return true;
}

bool
Map_Save2(FILE *fp)
{
	for (uint16 packed = 0; packed < MAP_SIZE_MAX * MAP_SIZE_MAX; packed++) {
		const Tile *t = &g_map[packed];
		const FogOfWarTile *f = &g_mapVisible[packed];
		uint16 timeout      = (f->timeout <= g_timerGame) ? 0 : (f->timeout - g_timerGame);
		uint16 spriteID     = ((f->overlaySpriteID & 0x7F) << 9) | (f->groundSpriteID & 0x1FF);
		uint8 houseID       = f->houseID;
		uint8 hasStructure  = f->hasStructure;

		/* Only store interesting tiles. */
		if (!g_map[packed].isUnveiled || spriteID == 0) continue;

		if ((timeout == 0) &&
				(f->groundSpriteID == t->groundSpriteID) &&
				((f->overlaySpriteID == t->overlaySpriteID) ||
				 (g_veiledSpriteID - 16 <= t->overlaySpriteID && t->overlaySpriteID <= g_veiledSpriteID)) &&
				(f->houseID == t->houseID) &&
				(f->hasStructure == t->hasStructure))
			continue;

		if (fwrite(&packed,         sizeof(uint16), 1, fp) != 1) return false;
		if (fwrite(&timeout,        sizeof(uint16), 1, fp) != 1) return false;
		if (fwrite(&spriteID,       sizeof(uint16), 1, fp) != 1) return false;
		if (fwrite(&houseID,        sizeof(uint8),  1, fp) != 1) return false;
		if (fwrite(&hasStructure,   sizeof(uint8),  1, fp) != 1) return false;
	}

	return true;
}
