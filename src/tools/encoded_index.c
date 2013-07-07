/**
 * @file src/tools/encoded_index.c
 *
 * Encoded index routines.
 *
 * An encoded index is a uint16 with one of the following formats:
 * <pre>
 *
 *  IT_NONE      = 00__ ____ ____ ____,
 *      if the encoded index is invalid;
 *
 *  IT_UNIT      = 01__ ____ ____ xxxx,
 *      where x is the index of the unit;
 *
 *  IT_STRUCTURE = 10__ ____ ____ xxxx,
 *      where x is the index of the structure; or
 *
 *  IT_TILE      = 11yy yyyy 1xxx xxx1,
 *      where (x, y) is the coordinate of the packed tile.
 *
 * </pre>
 */

#include <stdlib.h>

#include "encoded_index.h"

#include "coord.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../structure.h"
#include "../tile.h"
#include "../unit.h"

/**
 * @brief   f__167E_0005_0013_AF0C.
 * @details Removed guard for IT_UNIT.
 */
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

/**
 * @brief   f__167E_0088_001A_60ED.
 * @details Exact.
 */
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

/**
 * @brief   f__167E_00F3_001E_8CB3.
 * @details Simplified logic for IT_TILE.
 */
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

/**
 * @brief   f__167E_00B7_0034_F3DA.
 * @details Simplified logic for IT_TILE.
 */
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

/**
 * @brief   f__167E_0162_000D_A6D2.
 * @details Exact.
 */
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

/**
 * @brief   f__167E_01BB_0010_85F6.
 * @details IT_TILE is centred due to the '1' bit after the x/y in
 *          (encoded & 0x3F80) and (encoded & 0x7F).
 */
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

/**
 * @brief   f__167E_02D8_000C_4C9F.
 * @details Simplified logic.
 */
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

/**
 * @brief   f__167E_02AE_000C_CC85.
 * @details Exact.
 */
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

/**
 * @brief   f__167E_0284_000C_4C88.
 * @details Exact.
 */
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
