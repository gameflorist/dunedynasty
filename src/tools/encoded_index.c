/**
 * @file src/tools/encoded_index.c
 *
 * Encoded index routines.
 */

#include <stdlib.h>

#include "encoded_index.h"

#include "coord.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../structure.h"
#include "../tile.h"
#include "../unit.h"

bool
Tools_Index_IsValid(uint16 encoded)
{
	if (encoded == 0)
		return false;

	uint16 index = Tools_Index_Decode(encoded);
	switch (Tools_Index_GetType(encoded)) {
		case IT_UNIT:
			{
				const Unit *u = Unit_Get_ByIndex(index);
				return u->o.flags.s.used && u->o.flags.s.allocated;
			}

		case IT_STRUCTURE:
			{
				const Structure *s = Structure_Get_ByIndex(index);
				return s->o.flags.s.used;
			}

		case IT_TILE:
		case IT_NONE:
		default:
			return true;
	}

	return false;
}

enum IndexType
Tools_Index_GetType(uint16 encoded)
{
	switch (encoded & 0xC000) {
		case 0x4000: return IT_UNIT;
		case 0x8000: return IT_STRUCTURE;
		case 0xC000: return IT_TILE;
		default:     return IT_NONE;
	}
}

uint16
Tools_Index_Encode(uint16 index, enum IndexType type)
{
	switch (type) {
		case IT_TILE:
			{
				const uint16 x = (Tile_GetPackedX(index) << 1) | 0x01;
				const uint16 y = (Tile_GetPackedY(index) << 8) | 0x80;
				return (0xC000 | y | x);
			}

		case IT_UNIT:
			{
				const Unit *u = Unit_Get_ByIndex(index);
				return (u->o.flags.s.allocated) ? (0x4000 | index) : 0;
			}

		case IT_STRUCTURE:
			return (0x8000 | index);

		case IT_NONE:
		default:
			break;
	}

	return 0;
}

uint16
Tools_Index_Decode(uint16 encoded)
{
	if (Tools_Index_GetType(encoded) == IT_TILE) {
		const uint16 y = (encoded & 0x3F00) >> 8;
		const uint16 x = (encoded & 0x007E) >> 1;
		return Tile_PackXY(x, y);
	}
	else {
		return encoded & 0x3FFF;
	}
}

uint16
Tools_Index_GetPackedTile(uint16 encoded)
{
	uint16 index = Tools_Index_Decode(encoded);
	switch (Tools_Index_GetType(encoded)) {
		case IT_TILE:
			return index;

		case IT_UNIT:
			{
				const Unit *u = Unit_Get_ByIndex(index);
				return Tile_PackTile(u->o.position);
			}

		case IT_STRUCTURE:
			{
				const Structure *s = Structure_Get_ByIndex(index);
				return Tile_PackTile(s->o.position);
			}

		case IT_NONE:
		default:
			break;
	}

	return 0;
}

tile32
Tools_Index_GetTile(uint16 encoded)
{
	tile32 tile;

	uint16 index = Tools_Index_Decode(encoded);
	switch (Tools_Index_GetType(encoded)) {
		case IT_TILE:
			return Tile_UnpackTile(index);

		case IT_UNIT:
			{
				const Unit *u = Unit_Get_ByIndex(index);
				return u->o.position;
			}

		case IT_STRUCTURE:
			{
				const Structure *s = Structure_Get_ByIndex(index);
				const StructureInfo *si = &g_table_structureInfo[s->o.type];

				return Tile_AddTileDiff(s->o.position,
						g_table_structure_layoutTileDiff[si->layout]);
			}

		case IT_NONE:
		default:
			break;
	}

	tile.x = 0;
	tile.y = 0;
	return tile;
}

Object *
Tools_Index_GetObject(uint16 encoded)
{
	switch (Tools_Index_GetType(encoded)) {
		case IT_UNIT:
			{
				Unit *u = Tools_Index_GetUnit(encoded);
				return &u->o;
			}

		case IT_STRUCTURE:
			{
				Structure *s = Tools_Index_GetStructure(encoded);
				return &s->o;
			}

		case IT_NONE:
		case IT_TILE:
		default:
			break;
	}

	return NULL;
}

Structure *
Tools_Index_GetStructure(uint16 encoded)
{
	if (Tools_Index_GetType(encoded) == IT_STRUCTURE) {
		return Structure_Get_ByIndex(Tools_Index_Decode(encoded));
	}
	else {
		return NULL;
	}
}

Unit *
Tools_Index_GetUnit(uint16 encoded)
{
	if (Tools_Index_GetType(encoded) == IT_UNIT) {
		return Unit_Get_ByIndex(Tools_Index_Decode(encoded));
	}
	else {
		return NULL;
	}
}
