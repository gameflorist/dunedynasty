/** @file src/map.c Map routines. */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "os/common.h"
#include "os/math.h"

#include "map.h"

#include "animation.h"
#include "audio/audio.h"
#include "enhancement.h"
#include "explosion.h"
#include "gfx.h"
#include "gui/gui.h"
#include "gui/widget.h"
#include "house.h"
#include "newui/actionpanel.h"
#include "opendune.h"
#include "pool/pool.h"
#include "pool/unit.h"
#include "pool/house.h"
#include "pool/structure.h"
#include "scenario.h"
#include "sprites.h"
#include "structure.h"
#include "table/widgetinfo.h"
#include "team.h"
#include "timer/timer.h"
#include "tools/coord.h"
#include "tools/encoded_index.h"
#include "tools/random_general.h"
#include "tools/random_lcg.h"
#include "tile.h"
#include "unit.h"
#include "video/video.h"


uint16 g_mapSpriteID[64 * 64];

/* Map data.  In fog of war:
 * g_map[] corresponds to TRUE information, and the map scouted.
 * g_mapVisible[] corresponds to KNOWN information, and the map currently visible.
 */
Tile g_map[MAP_SIZE_MAX * MAP_SIZE_MAX];
FogOfWarTile g_mapVisible[MAP_SIZE_MAX * MAP_SIZE_MAX];

const uint8 g_functions[3][3] = {{0, 1, 0}, {2, 3, 0}, {0, 1, 0}};

static bool s_debugNoExplosionDamage = false;               /*!< When non-zero, explosions do no damage to their surrounding. */

/**
 * Map definitions.
 * Map sizes: [0] is 62x62, [1] is 32x32, [2] is 21x21.
 */
const MapInfo g_mapInfos[3] = {
	{ 1,  1, 62, 62},
	{16, 16, 32, 32},
	{21, 21, 21, 21}
};

bool
Map_InRangeX(int x)
{
	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];

	return (mapInfo->minX <= x && x < mapInfo->minX + mapInfo->sizeX);
}

bool
Map_InRangeY(int y)
{
	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];

	return (mapInfo->minY <= y && y < mapInfo->minY + mapInfo->sizeY);
}

static bool
Map_InRangePacked(uint16 packed)
{
	const int x = Tile_GetPackedX(packed);
	const int y = Tile_GetPackedY(packed);

	return Map_InRangeX(x) && Map_InRangeY(y);
}

int
Map_Clamp(int x)
{
	return clamp(0, x, MAP_SIZE_MAX - 1);
}

void
Map_MoveDirection(int dx, int dy)
{
	const ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];
	const int tilex = Tile_GetPackedX(g_viewportPosition);
	const int tiley = Tile_GetPackedY(g_viewportPosition);
	const int x = TILE_SIZE * tilex + g_viewport_scrollOffsetX + viewport->width / 2 + dx;
	const int y = TILE_SIZE * tiley + g_viewport_scrollOffsetY + viewport->height / 2 + dy;

	Map_CentreViewport(x, y);
}

void
Map_CentreViewport(int x, int y)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
	const int w = wi->width;
	const int h = wi->height;

	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
	int minx = TILE_SIZE * mapInfo->minX + w / 2;
	int miny = TILE_SIZE * mapInfo->minY + h / 2;
	int maxx = TILE_SIZE * (mapInfo->minX + mapInfo->sizeX) - w + (w / 2);
	int maxy = TILE_SIZE * (mapInfo->minY + mapInfo->sizeY) - h + (h / 2);
	int tilex, tiley;

	if (minx > maxx) minx = maxx = (minx + maxx) / 2;
	if (miny > maxy) miny = maxy = (miny + maxy) / 2;

	x = clamp(minx, x, maxx) - w / 2;
	y = clamp(miny, y, maxy) - h / 2;

	if (x >= 0) {
		tilex = x / TILE_SIZE;
		g_viewport_scrollOffsetX = x % TILE_SIZE;
	}
	else {
		tilex = 0;
		g_viewport_scrollOffsetX = x;
	}

	if (y >= 0) {
		tiley = y / TILE_SIZE;
		g_viewport_scrollOffsetY = y % TILE_SIZE;
	}
	else {
		tiley = 0;
		g_viewport_scrollOffsetY = y;
	}

	g_viewportPosition = Tile_PackXY(tilex, tiley);
}

/**
 * Sets the selection on given packed tile.
 *
 * @param packed The packed tile to set the selection on.
 */
void Map_SetSelection(uint16 packed)
{
	if (g_selectionType == SELECTIONTYPE_TARGET) return;

	if (g_selectionType == SELECTIONTYPE_PLACE) {
		g_selectionState = Structure_IsValidBuildLocation(packed, g_structureActiveType);
		g_selectionPosition = packed;
		return;
	}

	if ((packed != 0xFFFF && g_map[packed].overlaySpriteID != g_veiledSpriteID) || g_debugScenario) {
		Structure *s;

		s = Structure_Get_ByPackedTile(packed);
		if (s != NULL) {
			const StructureInfo *si;

			si = &g_table_structureInfo[s->o.type];
			if (s->o.houseID == g_playerHouseID && g_selectionType != SELECTIONTYPE_MENTAT) {
				GUI_DisplayHint(si->o.hintStringID, si->o.spriteID);
			}

			packed = Tile_PackTile(s->o.position);

			Map_SetSelectionSize(si->layout);

			Structure_UpdateMap(s);

			g_factoryWindowTotal = -1;
		} else {
			Map_SetSelectionSize(STRUCTURE_LAYOUT_1x1);
		}

		if (g_selectionType != SELECTIONTYPE_TARGET) {
			Unit *u;

			u = Unit_Get_ByPackedTile(packed);
			if (u != NULL) {
				if (u->o.type != UNIT_CARRYALL) {
					Unit_Select(u);
					Unit_DisplayStatusText(u);
				}
			} else {
				Unit_UnselectAll();
			}
		}
		g_selectionPosition = packed;
		return;
	}

	Map_SetSelectionSize(STRUCTURE_LAYOUT_1x1);
	g_selectionPosition = packed;
	return;
}

/**
 * Sets the selection size for the given layout.
 *
 * @param layout The layout to determine selection size from.
 * @return The previous layout.
 * @see StructureLayout
 */
void Map_SetSelectionSize(uint16 layout)
{
	uint16 oldPosition;

	if (layout & 0x80) return;

	oldPosition = Map_SetSelectionObjectPosition(0xFFFF);

	g_selectionWidth  = g_table_structure_layoutSize[layout].width;
	g_selectionHeight = g_table_structure_layoutSize[layout].height;

	Map_SetSelectionObjectPosition(oldPosition);
}

/**
 * Sets the selection object to the given position.
 *
 * @param packed The position to set.
 * @return The previous position.
 */
uint16 Map_SetSelectionObjectPosition(uint16 packed)
{
	static uint16 selectionPosition = 0xFFFF;

	uint16 oldPacked = selectionPosition;

	if (packed == oldPacked) return oldPacked;

	selectionPosition = packed;

	return oldPacked;
}

/**
 * Update the minimap position.
 *
 * @param packed The new position.
 * @param forceUpdate If true force the update even if the position didn't change.
 */
void Map_UpdateMinimapPosition(uint16 packed, bool forceUpdate)
{
#if 0
	/* Border tiles of the viewport relative to the top-left. */
	static const uint16 viewportBorder[] = {
		0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E,
		0x0040, 0x004E,
		0x0080, 0x008E,
		0x00C0, 0x00CE,
		0x0100, 0x010E,
		0x0140, 0x014E,
		0x0180, 0x018E,
		0x01C0, 0x01CE,
		0x0200, 0x020E,
		0x0240, 0x0241, 0x0242, 0x0243, 0x0244, 0x0245, 0x0246, 0x0247, 0x0248, 0x0249, 0x024A, 0x024B, 0x024C, 0x024D, 0x024E,
		0xFFFF
	};

	static uint16 minimapPreviousPosition = 0;

	bool cleared;
	Screen oldScreenID;

	if (packed != 0xFFFF && packed == minimapPreviousPosition && !forceUpdate) return;
	if (g_selectionType == SELECTIONTYPE_MENTAT) return;

	oldScreenID = GFX_Screen_SetActive(SCREEN_1);

	cleared = false;

	if (minimapPreviousPosition != 0xFFFF && minimapPreviousPosition != packed) {
		const uint16 *m;

		cleared = true;

		for (m = viewportBorder; *m != 0xFFFF; m++) {
			uint16 curPacked;

			curPacked = minimapPreviousPosition + *m;
			BitArray_Clear(g_displayedMinimap, curPacked);

			GUI_Widget_Viewport_DrawTile(curPacked);
		}
	}

	if (packed != 0xFFFF && (packed != minimapPreviousPosition || forceUpdate)) {
		const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP];
		const uint16 *m;
		uint16 mapScale;
		const MapInfo *mapInfo;
		uint16 left, top, right, bottom;

		mapScale = g_scenario.mapScale;
		mapInfo = &g_mapInfos[mapScale];

		left   = (Tile_GetPackedX(packed) - mapInfo->minX) * (mapScale + 1) + wi->offsetX;
		right  = left + mapScale * 15 + 14;
		top    = (Tile_GetPackedY(packed) - mapInfo->minY) * (mapScale + 1) + wi->offsetY;
		bottom = top + mapScale * 10 + 9;

		GUI_DrawWiredRectangle(left, top, right, bottom, 15);

		for (m = viewportBorder; *m != 0xFFFF; m++) {
			uint16 curPacked;

			curPacked = packed + *m;
			BitArray_Set(g_displayedMinimap, curPacked);
		}
	}

	if (cleared && oldScreenID == SCREEN_0) {
		GUI_Mouse_Hide_Safe();
		GUI_Screen_Copy(32, 136, 32, 136, 8, 64, SCREEN_1, SCREEN_0);
		GUI_Mouse_Show_Safe();
	}

	GFX_Screen_SetActive(oldScreenID);

	minimapPreviousPosition = packed;
#else
	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
	const WidgetInfo *minimap = &g_table_gameWidgetInfo[GAME_WIDGET_MINIMAP];
	const WidgetInfo *viewport = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
	const int tx = Tile_GetPackedX(packed) - mapInfo->minX;
	const int ty = Tile_GetPackedY(packed) - mapInfo->minY;
	const int w = ceilf((float)viewport->width / TILE_SIZE);
	const int h = ceilf((float)viewport->height / TILE_SIZE);
	VARIABLE_NOT_USED(forceUpdate);

	int x1 = minimap->offsetX + (float)tx * minimap->width / mapInfo->sizeX;
	int y1 = minimap->offsetY + (float)ty * minimap->height / mapInfo->sizeY;
	int x2 = minimap->offsetX + (float)(tx + w - 1) * minimap->width / mapInfo->sizeX - 1;
	int y2 = minimap->offsetY + (float)(ty + h - 1) * minimap->height / mapInfo->sizeY - 1;

	x1 = clamp(minimap->offsetX, x1, minimap->offsetX + minimap->width - 1);
	x2 = clamp(minimap->offsetX, x2, minimap->offsetX + minimap->width - 1);
	y1 = clamp(minimap->offsetY, y1, minimap->offsetY + minimap->height - 1);
	y2 = clamp(minimap->offsetY, y2, minimap->offsetY + minimap->height - 1);

	Prim_Rect_i(x1, y1, x2, y2, 15);
#endif
}

/**
 * Checks if the given position is inside the map.
 *
 * @param position The tile (packed) to check.
 * @return True if the position is valid.
 */
bool Map_IsValidPosition(uint16 position)
{
	uint16 x, y;
	const MapInfo *mapInfo;

	if ((position & 0xC000) != 0) return false;

	x = Tile_GetPackedX(position);
	y = Tile_GetPackedY(position);

	mapInfo = &g_mapInfos[g_scenario.mapScale];

	return (mapInfo->minX <= x && x < (mapInfo->minX + mapInfo->sizeX) && mapInfo->minY <= y && y < (mapInfo->minY + mapInfo->sizeY));
}

/**
 * Check if a position is unveiled (the fog is removed).
 *
 * @param position For which position to check.
 * @return True if and only if the position is unveiled.
 */
bool Map_IsPositionUnveiled(uint16 position)
{
	Tile *t;

	if (g_debugScenario) return true;

	t = &g_map[position];

	if (!t->isUnveiled) return false;
	if (!Sprite_IsUnveiled(t->overlaySpriteID)) return false;

	return true;
}

/**
 * Check if a position is in the viewport.
 *
 * @param position For which position to check.
 * @param retX Pointer to X inside the viewport.
 * @param retY Pointer to Y inside the viewport.
 * @return True if and only if the position is in the viewport.
 */
bool Map_IsPositionInViewport(tile32 position, int *retX, int *retY)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];
	const int x = (position.x >> 4) - (Tile_GetPackedX(g_viewportPosition) * TILE_SIZE) - g_viewport_scrollOffsetX;
	const int y = (position.y >> 4) - (Tile_GetPackedY(g_viewportPosition) * TILE_SIZE) - g_viewport_scrollOffsetY;

	if (retX != NULL) *retX = x;
	if (retY != NULL) *retY = y;

	return ((-TILE_SIZE <= x && x < TILE_SIZE + wi->width) &&
	        (-TILE_SIZE <= y && y < TILE_SIZE + wi->height));
}

static bool Map_UpdateWall(uint16 packed)
{
	Tile *t;

	if (Map_GetLandscapeType(packed) != LST_WALL) return false;

	t = &g_map[packed];

	t->groundSpriteID = g_mapSpriteID[packed] & 0x1FF;

	if (Map_IsPositionUnveiled(packed)) t->overlaySpriteID = g_wallSpriteID;

	Structure_ConnectWall(packed, true);
	Map_Update(packed, 0, false);

	/* ENHANCEMENT -- stop targetting the wall after it has been destroyed. */
	if (enhancement_fix_firing_logic) {
		const uint16 encoded = Tools_Index_Encode(packed, IT_TILE);
		Unit_UntargetEncodedIndex(encoded);
	}

	return true;
}

/**
 * Make an explosion on the given position, of a certain type. All units in the
 *  neighbourhoud get an amount of damage related to their distance to the
 *  explosion.
 * @param type The type of explosion.
 * @param position The position of the explosion.
 * @param hitpoints The amount of hitpoints to give people in the neighbourhoud.
 * @param unitOriginEncoded The unit that fired the bullet.
 */
void Map_MakeExplosion(uint16 type, tile32 position, uint16 hitpoints, uint16 unitOriginEncoded)
{
	uint16 reactionDistance = (type == EXPLOSION_DEATH_HAND) ? 32 : 16;
	uint16 positionPacked = Tile_PackTile(position);

	if (!s_debugNoExplosionDamage && hitpoints != 0) {
		PoolFindStruct find;
		find.houseID = HOUSE_INVALID;
		find.index   = 0xFFFF;
		find.type    = 0xFFFF;

		while (true) {
			const UnitInfo *ui;
			uint16 distance;
			Team *t;
			Unit *u;
			Unit *us;
			Unit *attack;

			u = Unit_Find(&find);
			if (u == NULL) break;

			ui = &g_table_unitInfo[u->o.type];

			distance = Tile_GetDistance(position, u->o.position) >> 4;
			if (distance >= reactionDistance) continue;

			if (!(u->o.type == UNIT_SANDWORM && type == EXPLOSION_SANDWORM_SWALLOW) && u->o.type != UNIT_FRIGATE) {
				Unit_Damage(u, hitpoints >> (distance >> 2), 0);
			}

			if (u->o.houseID == g_playerHouseID) continue;

			us = Tools_Index_GetUnit(unitOriginEncoded);
			if (us == NULL) continue;
			if (us == u) continue;
			if (House_AreAllied(Unit_GetHouseID(u), Unit_GetHouseID(us))) continue;

			t = Unit_GetTeam(u);
			if (t != NULL) {
				const UnitInfo *targetInfo;
				Unit *target;

				if (t->action == TEAM_ACTION_STAGING) {
					Unit_RemoveFromTeam(u);
					Unit_SetAction(u, ACTION_HUNT);
					continue;
				}

				target = Tools_Index_GetUnit(t->target);
				if (target == NULL) continue;

				targetInfo = &g_table_unitInfo[target->o.type];
				if (targetInfo->bulletType == UNIT_INVALID) t->target = unitOriginEncoded;
				continue;
			}

			if (u->o.type == UNIT_HARVESTER) {
				const UnitInfo *uis = &g_table_unitInfo[us->o.type];

				if (uis->movementType == MOVEMENT_FOOT && u->targetMove == 0) {
					if (u->actionID != ACTION_MOVE) Unit_SetAction(u, ACTION_MOVE);
					u->targetMove = unitOriginEncoded;
					continue;
				}
			}

			if (ui->bulletType == UNIT_INVALID) continue;

			if (u->actionID == ACTION_GUARD && u->o.flags.s.byScenario) {
				Unit_SetAction(u, ACTION_HUNT);
			}

			if (u->targetAttack != 0 && u->actionID != ACTION_HUNT) continue;

			attack = Tools_Index_GetUnit(u->targetAttack);
			if (attack != NULL) {
				uint16 packed = Tile_PackTile(u->o.position);
				if (Tile_GetDistancePacked(Tools_Index_GetPackedTile(u->targetAttack), packed) <= ui->fireDistance) continue;
			}

			Unit_SetTarget(u, unitOriginEncoded);
		}
	}

	if (!s_debugNoExplosionDamage && hitpoints != 0) {
		Structure *s = Structure_Get_ByPackedTile(positionPacked);

		if (s != NULL) {
			if (type == EXPLOSION_IMPACT_LARGE) {
				const StructureInfo *si = &g_table_structureInfo[s->o.type];

				if (si->o.hitpoints / 2 > s->o.hitpoints) {
					type = EXPLOSION_SMOKE_PLUME;
				}
			}

			Structure_HouseUnderAttack(s->o.houseID);
			Structure_Damage(s, hitpoints, 0);
		}
	}

	if (Map_GetLandscapeType(positionPacked) == LST_WALL && hitpoints != 0) {
		bool loc22 = false;

		if (g_table_structureInfo[STRUCTURE_WALL].o.hitpoints <= hitpoints) loc22 = true;

		if (!loc22) {
			uint16 loc24 = hitpoints * 256 / g_table_structureInfo[STRUCTURE_WALL].o.hitpoints;

			if (Tools_Random_256() <= loc24) loc22 = true;
		}

		if (loc22) Map_UpdateWall(positionPacked);
	}

	Explosion_Start(type, position);
}

/**
 * Type of landscape for the landscape sprites.
 *
 * 0=normal sand, 1=partial rock, 5=mostly rock, 4=entirely rock,
 * 3=partial sand dunes, 2=entirely sand dunes, 7=partial mountain,
 * 6=entirely mountain, 8=spice, 9=thick spice
 * @see Map_GetLandscapeType
 */
static const uint16 _landscapeSpriteMap[81] = {
	0, 1, 1, 1, 5, 1, 5, 5, 5, 5, /* Sprites 127-136 */
	5, 5, 5, 5, 5, 5, 4, 3, 3, 3, /* Sprites 137-146 */
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* Sprites 147-156 */
	3, 3, 2, 7, 7, 7, 7, 7, 7, 7, /* Sprites 157-166 */
	7, 7, 7, 7, 7, 7, 7, 7, 6, 8, /* Sprites 167-176 */
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, /* Sprites 177-186 */
	8, 8, 8, 8, 8, 9, 9, 9, 9, 9, /* Sprites 187-196 */
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /* Sprites 197-206 */
	9,                            /* Sprite  207 (bloom sprites 208 and 209 are already caught). */
};

/**
 * Get type of landscape of a tile.
 *
 * @param packed The packed tile to examine.
 * @return The type of landscape at the tile.
 */
static enum LandscapeType
Map_GetLandscapeType_BySpriteID(uint16 spriteID, bool hasStructure)
{
	int16 spriteOffset;

	if (spriteID == g_builtSlabSpriteID) return LST_CONCRETE_SLAB;

	if (spriteID == g_bloomSpriteID || spriteID == g_bloomSpriteID + 1) return LST_BLOOM_FIELD;

	if (g_wallSpriteID < spriteID && spriteID < g_wallSpriteID + 75) return LST_WALL;

	/* if (t->overlaySpriteID == g_wallSpriteID) return LST_DESTROYED_WALL; */

	/* if (Structure_Get_ByPackedTile(packed) != NULL) return LST_STRUCTURE; */
	if (hasStructure) return LST_STRUCTURE;

	spriteOffset = spriteID - g_landscapeSpriteID; /* Offset in the landscape icon group. */
	if (spriteOffset < 0 || spriteOffset > 80) return LST_ENTIRELY_ROCK;

	return _landscapeSpriteMap[spriteOffset];
}

uint16 Map_GetLandscapeType(uint16 packed)
{
	Tile *t = &g_map[packed];

	if (t->overlaySpriteID == g_wallSpriteID) return LST_DESTROYED_WALL;

	return Map_GetLandscapeType_BySpriteID(t->groundSpriteID, t->hasStructure);
}

enum LandscapeType
Map_GetLandscapeTypeVisible(uint16 packed)
{
	FogOfWarTile *t = &g_mapVisible[packed];

	return Map_GetLandscapeType_BySpriteID(t->groundSpriteID, t->hasStructure);
}

enum LandscapeType
Map_GetLandscapeTypeOriginal(uint16 packed)
{
	return Map_GetLandscapeType_BySpriteID(g_mapSpriteID[packed] & 0x7FFF, false);
}

#if 0
/**
 * Checks wether a packed tile is visible in the viewport.
 *
 * @param packed The packed tile.
 * @return True if the tile is visible.
 */
static bool Map_IsTileVisible(uint16 packed)
{
	uint8 x, y;
	uint8 x2, y2;

	x = Tile_GetPackedX(packed);
	y = Tile_GetPackedY(packed);
	x2 = Tile_GetPackedX(g_minimapPosition);
	y2 = Tile_GetPackedY(g_minimapPosition);

	return x >= x2 && x < x2 + 15 && y >= y2 && y < y2 + 10;
}
#endif

/**
 * Updates ??.
 *
 * @param packed The packed tile.
 * @param type The type of update.
 * @param ignoreInvisible Wether to ignore tile visibility check.
 */
void Map_Update(uint16 packed, uint16 type, bool ignoreInvisible)
{
	VARIABLE_NOT_USED(packed);
	VARIABLE_NOT_USED(type);
	VARIABLE_NOT_USED(ignoreInvisible);
}

/**
 * Make a deviator missile explosion on the given position, of a certain type. All units in the
 *  given radius may become deviated.
 * @param type The type of explosion.
 * @param position The position of the explosion.
 * @param radius The radius.
 * @param houseID House controlling the deviator.
 */
void Map_DeviateArea(uint16 type, tile32 position, uint16 radius, uint8 houseID)
{
	PoolFindStruct find;

	Explosion_Start(type, position);

	find.type    = 0xFFFF;
	find.index   = 0xFFFF;
	find.houseID = HOUSE_INVALID;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);

		if (u == NULL) break;
		if (Tile_GetDistance(position, u->o.position) / 16 >= radius) continue;

		Unit_Deviate(u, 0, houseID);
	}
}

/**
 * Perform a bloom explosion, filling the area with spice.
 * @param packed Center position.
 * @param houseID %House causing the explosion.
 */
void Map_Bloom_ExplodeSpice(uint16 packed, uint8 houseID)
{
	if (g_validateStrictIfZero == 0) {
		Unit_Remove(Unit_Get_ByPackedTile(packed));
		g_map[packed].groundSpriteID = g_mapSpriteID[packed] & 0x1FF;
		Map_MakeExplosion(EXPLOSION_SPICE_BLOOM_TREMOR, Tile_UnpackTile(packed), 0, 0);
	}

	if (houseID == g_playerHouseID)
		Audio_PlayVoice(VOICE_SPICE_BLOOM_LOCATED);

	Map_FillCircleWithSpice(packed, 5);

	/* ENHANCEMENT -- stop targetting the bloom after it has exploded. */
	if (enhancement_fix_firing_logic) {
		const uint16 encoded = Tools_Index_Encode(packed, IT_TILE);
		Unit_UntargetEncodedIndex(encoded);
	}
}

/**
 * Fill a circular area with spice.
 * @param packed Center position of the area.
 * @param radius Radius of the circle.
 */
void Map_FillCircleWithSpice(uint16 packed, uint16 radius)
{
	uint16 x;
	uint16 y;
	int i;
	int j;

	if (radius == 0) return;

	x = Tile_GetPackedX(packed);
	y = Tile_GetPackedY(packed);

	for (i = -radius; i <= radius; i++) {
		for (j = -radius; j <= radius; j++) {
			uint16 curPacked = Tile_PackXY(x + j, y + i);
			uint16 distance  = Tile_GetDistancePacked(packed, curPacked);

			if (distance > radius) continue;

			if (distance == radius && (Tools_Random_256() & 1) == 0) continue;

			if (Map_GetLandscapeType(curPacked) == LST_SPICE) continue;

			Map_ChangeSpiceAmount(curPacked, 1);
		}
	}

	Map_ChangeSpiceAmount(packed, 1);
}

/**
 * Fixes edges of spice / thick spice to show sand / normal spice for better looks.
 * @param packed Position to check and possible fix edges of.
 */
static void Map_FixupSpiceEdges(uint16 packed)
{
	/* Relative steps in the map array for moving up, right, down, left. */
	static const int16 _mapDifference[] = {-64, 1, 64, -1};

	uint16 type;
	uint16 spriteID;

	packed &= 0xFFF;
	type = Map_GetLandscapeType(packed);
	spriteID = 0;

	if (type == LST_SPICE || type == LST_THICK_SPICE) {
		uint8 i;

		for (i = 0; i < 4; i++) {
			uint16 curPacked = packed + _mapDifference[i];
			uint16 curType;

			if (Tile_IsOutOfMap(curPacked)) {
				if (type == LST_SPICE || type == LST_THICK_SPICE) spriteID |= (1 << i);
				continue;
			}

			curType = Map_GetLandscapeType(curPacked);

			if (type == LST_SPICE) {
				if (curType == LST_SPICE || curType == LST_THICK_SPICE) spriteID |= (1 << i);
				continue;
			}

			if (curType == LST_THICK_SPICE) spriteID |= (1 << i);
		}

		spriteID += (type == LST_SPICE) ? 49 : 65;

		spriteID = g_iconMap[g_iconMap[ICM_ICONGROUP_LANDSCAPE] + spriteID] & 0x1FF;
		g_mapSpriteID[packed] = 0x8000 | spriteID;
		g_map[packed].groundSpriteID = spriteID;
	}

	Map_Update(packed, 0, false);
}

/**
 * Change amount of spice at \a packed position.
 * @param packed Position in the world to modify.
 * @param dir Direction of change, > 0 means add spice, < 0 means remove spice.
 */
void Map_ChangeSpiceAmount(uint16 packed, int16 dir)
{
	uint16 type;
	uint16 spriteID;

	if (dir == 0) return;

	type = Map_GetLandscapeType(packed);

	if (type == LST_THICK_SPICE && dir > 0) return;
	if (type != LST_SPICE && type != LST_THICK_SPICE && dir < 0) return;
	if (type != LST_NORMAL_SAND && type != LST_ENTIRELY_DUNE && type != LST_SPICE && dir > 0) return;

	if (dir > 0) {
		type = (type == LST_SPICE) ? LST_THICK_SPICE : LST_SPICE;
	} else {
		type = (type == LST_THICK_SPICE) ? LST_SPICE : LST_NORMAL_SAND;
	}

	spriteID = 0;
	if (type == LST_SPICE) spriteID = 49;
	if (type == LST_THICK_SPICE) spriteID = 65;

	spriteID = g_iconMap[g_iconMap[ICM_ICONGROUP_LANDSCAPE] + spriteID] & 0x1FF;
	g_mapSpriteID[packed] = 0x8000 | spriteID;
	g_map[packed].groundSpriteID = spriteID;

	Map_FixupSpiceEdges(packed);
	Map_FixupSpiceEdges(packed + 1);
	Map_FixupSpiceEdges(packed - 1);
	Map_FixupSpiceEdges(packed - 64);
	Map_FixupSpiceEdges(packed + 64);
}

/**
 * A unit drove over a special bloom, which can either give credits, a friendly
 *  Trike, an enemy Trike, or an enemy Infantry.
 * @param packed The tile where the bloom is on.
 * @param houseID The HouseID that is driving over the bloom.
 */
void Map_Bloom_ExplodeSpecial(uint16 packed, uint8 houseID)
{
	House *h;
	PoolFindStruct find;
	uint8 enemyHouseID;

	h = House_Get_ByIndex(houseID);

	g_map[packed].groundSpriteID = g_landscapeSpriteID;
	g_mapSpriteID[packed] = 0x8000 | g_landscapeSpriteID;

	Map_Update(packed, 0, false);

	enemyHouseID = houseID;

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	/* Find a house that belongs to the enemy */
	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		if (u->o.houseID == houseID) continue;

		enemyHouseID = u->o.houseID;
		break;
	}

	switch (Tools_Random_256() & 0x3) {
		case 0:
			h->credits += Tools_RandomLCG_Range(150, 400);
			break;

		case 1: {
			tile32 position = Tile_UnpackTile(packed);

			position = Tile_MoveByRandom(position, 16, true);

			/* ENHANCEMENT -- Dune2 inverted houseID and typeID arguments. */
			Unit_Create(UNIT_INDEX_INVALID, UNIT_TRIKE, houseID, position, Tools_Random_256());
			break;
		}

		case 2: {
			tile32 position = Tile_UnpackTile(packed);
			Unit *u;

			position = Tile_MoveByRandom(position, 16, true);

			/* ENHANCEMENT -- Dune2 inverted houseID and typeID arguments. */
			u = Unit_Create(UNIT_INDEX_INVALID, UNIT_TRIKE, enemyHouseID, position, Tools_Random_256());

			if (u != NULL) Unit_SetAction(u, ACTION_HUNT);
			break;
		}

		case 3: {
			tile32 position = Tile_UnpackTile(packed);
			Unit *u;

			position = Tile_MoveByRandom(position, 16, true);

			/* ENHANCEMENT -- Dune2 inverted houseID and typeID arguments. */
			u = Unit_Create(UNIT_INDEX_INVALID, UNIT_INFANTRY, enemyHouseID, position, Tools_Random_256());

			if (u != NULL) Unit_SetAction(u, ACTION_HUNT);
			break;
		}

		default: break;
	}
}

/**
 * Find a tile close the a LocationID described position (North, Enemy Base, ..).
 *
 * @param locationID Value between 0 and 7 to indicate where the tile should be.
 * @param houseID The HouseID looking for a tile (to get an idea of Enemy Base).
 * @return The tile requested.
 */
uint16 Map_FindLocationTile(uint16 locationID, uint8 houseID)
{
	static int16 mapBase[3] = {1, -2, -2};

	uint16 ret = 0;
	uint16 mapOffset;

	mapOffset = mapBase[g_scenario.mapScale];

	if (locationID == 6) { /* Enemy Base */
		PoolFindStruct find;

		find.houseID = HOUSE_INVALID;
		find.index   = 0xFFFF;
		find.type    = 0xFFFF;

		/* Find the house of an enemy */
		while (true) {
			Structure *s;

			s = Structure_Find(&find);
			if (s == NULL) break;

			if (s->o.houseID == houseID) continue;

			houseID = s->o.houseID;
			break;
		}
	}

	while (ret == 0) {
		switch (locationID) {
			case 0: { /* North */
				const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
				ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX - 2), mapInfo->minY + mapOffset);
				break;
			}

			case 1: { /* East */
				const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
				ret = Tile_PackXY(mapInfo->minX + mapInfo->sizeX - mapOffset, mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY - 2));
				break;
			}

			case 2: { /* South */
				const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
				ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX - 2), mapInfo->minY + mapInfo->sizeY - mapOffset);
				break;
			}

			case 3: { /* West */
				const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
				ret = Tile_PackXY(mapInfo->minX + mapOffset, mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY - 2));
				break;
			}

			case 4: { /* Air */
				const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
				ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX), mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY));
				if (houseID == g_playerHouseID && !Map_IsValidPosition(ret)) ret = 0;
				break;
			}

			case 5: /* Visible */
				ret = Tile_PackXY(Tile_GetPackedX(g_viewportPosition) + Tools_RandomLCG_Range(0, 14), Tile_GetPackedY(g_viewportPosition) + Tools_RandomLCG_Range(0, 9));
				if (houseID == g_playerHouseID && !Map_IsValidPosition(ret)) ret = 0;
				break;

			case 6: /* Enemy Base */
			case 7: { /* Home Base */
				PoolFindStruct find;
				Structure *s;

				find.houseID = houseID;
				find.index   = 0xFFFF;
				find.type    = 0xFFFF;

				s = Structure_Find(&find);

				if (s != NULL) {
					ret = Tile_PackTile(Tile_MoveByRandom(s->o.position, 120, true));
				} else {
					Unit *u;

					find.houseID = houseID;
					find.index   = 0xFFFF;
					find.type    = 0xFFFF;

					u = Unit_Find(&find);

					if (u != NULL) {
						ret = Tile_PackTile(Tile_MoveByRandom(u->o.position, 120, true));
					} else {
						const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
						ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX), mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY));
					}
				}

				if (houseID == g_playerHouseID && !Map_IsValidPosition(ret)) ret = 0;
				break;
			}

			default: return 0;
		}

		ret &= 0xFFF;
		if (ret != 0 && Object_GetByPackedTile(ret) != NULL) ret = 0;
	}

	return ret;
}

/**
 * Around a position, run a certain function for all tiles within a certain radius.
 *
 * @note Radius is in a 1/4th of a tile unit.
 *
 * @param radius The radius of the to-update tiles.
 * @param position The position to go from.
 * @param unit The unit to update for (can be NULL if function < 2).
 * @param function The function to call.
 */
void Map_UpdateAround(uint16 radius, tile32 position, Unit *unit, uint8 function)
{
	static const uint16 tileOffsets[] = {
		0x0080, 0x0088, 0x0090, 0x0098,
		0x00A0, 0x00A8, 0x00B0, 0x00B8,
		0x00C0, 0x00C8, 0x00D0, 0x00D8,
		0x00E0, 0x00E8, 0x00F0, 0x00F8,
		0x0100, 0x0180
	};

	int16 i, j;
	tile32 diff;
	uint16 lastPacked;

	if (radius == 0 || (position.x == 0 && position.y == 0)) return;

	radius--;

	/* If radius is bigger or equal than 32, update all tiles in a 5x5 grid around the unit. */
	if (radius >= 32) {
		uint16 x = Tile_GetPosX(position);
		uint16 y = Tile_GetPosY(position);

		for (i = -2; i <= 2; i++) {
			for (j = -2; j <= 2; j++) {
				uint16 curPacked;

				if (x + i < 0 || x + i >= 64 || y + j < 0 || y + j >= 64) continue;

				curPacked = Tile_PackXY(x + i, y + j);

				switch (function) {
					case 0: Map_Update(curPacked, 0, false); break;
					case 1: Map_Update(curPacked, 3, false); break;
					case 2: Unit_RemoveFromTile(unit, curPacked); break;
					case 3: Unit_AddToTile(unit, curPacked); break;
					default: break;
				}
			}
		}
		return;
	}

	radius = max(radius, 15);
	position.x -= tileOffsets[radius - 15];
	position.y -= tileOffsets[radius - 15];

	diff.x = 0;
	diff.y = 0;
	lastPacked = 0;

	i = 0;
	do {
		tile32 curTile = Tile_AddTileDiff(position, diff);

		if (Tile_IsValid(curTile)) {
			uint16 curPacked = Tile_PackTile(curTile);

			if (curPacked != lastPacked) {
				switch (function) {
					case 0: Map_Update(curPacked, 0, false); break;
					case 1: Map_Update(curPacked, 3, false); break;
					case 2: Unit_RemoveFromTile(unit, curPacked); break;
					case 3: Unit_AddToTile(unit, curPacked); break;
					default: break;
				}

				lastPacked = curPacked;
			}
		}

		if (i == 8) break;
		diff = g_table_tilediff[radius + 1][i++];
	} while ((diff.x != 0) || (diff.y != 0));
}

/**
 * Search for spice around a position. Thick spice is preferred if it is not too far away.
 * @param packed Center position.
 * @param radius Radius of the search.
 * @return Best position with spice, or \c 0 if no spice found.
 */
uint16 Map_SearchSpice(uint16 packed, uint16 radius)
{
	uint16 radius1;
	uint16 radius2;
	uint16 packed1;
	uint16 packed2;
	uint16 xmin;
	uint16 xmax;
	uint16 ymin;
	uint16 ymax;
	const MapInfo *mapInfo;
	uint16 x;
	uint16 y;
	bool found;

	radius1 = radius + 1;
	radius2 = radius + 1;
	packed1 = packed;
	packed2 = packed;

	found = false;

	mapInfo = &g_mapInfos[g_scenario.mapScale];

	xmin = max(Tile_GetPackedX(packed) - radius, mapInfo->minX);
	xmax = min(Tile_GetPackedX(packed) + radius, mapInfo->minX + mapInfo->sizeX - 1);
	ymin = max(Tile_GetPackedY(packed) - radius, mapInfo->minY);
	ymax = min(Tile_GetPackedY(packed) + radius, mapInfo->minY + mapInfo->sizeY - 1);

	for (y = ymin; y <= ymax; y++) {
		for (x = xmin; x <= xmax; x++) {
			uint16 curPacked = Tile_PackXY(x, y);
			uint16 type;
			uint16 distance;

			if (!Map_IsValidPosition(curPacked)) continue;
			if (g_map[curPacked].hasStructure) continue;
			if (Unit_Get_ByPackedTile(curPacked) != NULL) continue;

			type = Map_GetLandscapeType(curPacked);
			distance = Tile_GetDistancePacked(curPacked, packed);

			if (type == LST_THICK_SPICE && distance < 4) {
				found = true;

				if (distance <= radius2) {
					radius2 = distance;
					packed2 = curPacked;
				}
			}

			if (type == LST_SPICE) {
				found = true;

				if (distance <= radius1) {
					radius1 = distance;
					packed1 = curPacked;
				}
			}
		}
	}

	if (!found) return 0;

	return (radius2 <= radius) ? packed2 : packed1;
}

#if 0
void Map_SelectNext(bool getNext)
{
	PoolFindStruct find;
	Object *selected = NULL;
	Object *previous = NULL;
	Object *next = NULL;
	Object *first = NULL;
	Object *last = NULL;
	bool hasPrevious = false;
	bool hasNext = false;

	if (g_unitSelected != NULL) {
		if (Map_IsTileVisible(Tile_PackTile(g_unitSelected->o.position))) selected = &g_unitSelected->o;
	} else {
		Structure *s;

		s = Structure_Get_ByPackedTile(g_selectionPosition);

		if (s != NULL && Map_IsTileVisible(Tile_PackTile(s->o.position))) selected = &s->o;
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		if (!g_table_unitInfo[u->o.type].o.flags.tabSelectable) continue;

		if (!Map_IsTileVisible(Tile_PackTile(u->o.position))) continue;

		if ((u->o.seenByHouses & (1 << g_playerHouseID)) == 0) continue;

		if (first == NULL) first = &u->o;
		last = &u->o;
		if (selected == NULL) selected = &u->o;

		if (selected == &u->o) {
			hasPrevious = true;
			continue;
		}

		if (!hasPrevious) {
			previous = &u->o;
			continue;
		}

		if (!hasNext) {
			next = &u->o;
			hasNext = true;
		}
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;

		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;

		if (!Map_IsTileVisible(Tile_PackTile(s->o.position))) continue;

		if ((s->o.seenByHouses & (1 << g_playerHouseID)) == 0) continue;

		if (first == NULL) first = &s->o;
		last = &s->o;
		if (selected == NULL) selected = &s->o;

		if (selected == &s->o) {
			hasPrevious = true;
			continue;
		}

		if (!hasPrevious) {
			previous = &s->o;
			continue;
		}

		if (!hasNext) {
			next = &s->o;
			hasNext = true;
		}
	}

	if (previous == NULL) previous = last;
	if (next == NULL) next = first;
	if (previous == NULL) previous = next;
	if (next == NULL) next = previous;

	selected = getNext ? next : previous;

	if (selected == NULL) return;

	Map_SetSelection(Tile_PackTile(selected->position));
}
#endif

/**
 * After unveiling, check neighbour tiles. This function handles one neighbour.
 * @param packed The neighbour tile of an unveiled tile.
 */
static void Map_UnveilTile_Neighbour(uint16 packed)
{
	uint16 spriteID;
	Tile *t;

	if (!Map_InRangePacked(packed)) return;

	t = &g_map[packed];

	spriteID = 15;
	if (t->isUnveiled) {
		int i;

		if (g_validateStrictIfZero == 0 && Sprite_IsUnveiled(t->overlaySpriteID)) return;

		spriteID = 0;

		for (i = 0; i < 4; i++) {
			static const int16 mapOffset[] = {-64, 1, 64, -1};
			uint16 neighbour = packed + mapOffset[i];

			if (Tile_IsOutOfMap(neighbour)) {
				spriteID |= 1 << i;
				continue;
			}

			if (!g_map[neighbour].isUnveiled) spriteID |= 1 << i;
		}
	}

	if (spriteID != 0) {
		if (spriteID != 15) {
			Unit *u = Unit_Get_ByPackedTile(packed);
			if (u != NULL) Unit_HouseUnitCount_Add(u, g_playerHouseID);
		}

		spriteID = g_iconMap[g_iconMap[ICM_ICONGROUP_FOG_OF_WAR] + spriteID];
	}

	t->overlaySpriteID = spriteID;

	Map_Update(packed, 0, false);
}

/**
 * Unveil a tile for a House.
 * @param packed The tile to unveil.
 * @param houseID The house to unveil for.
 * @return True if tile was freshly unveiled.
 */
bool Map_UnveilTile(uint16 packed, uint8 houseID)
{
	Structure *s;
	Unit *u;
	Tile *t;

	if (houseID != g_playerHouseID) return false;
	if (Tile_IsOutOfMap(packed)) return false;

	t = &g_map[packed];
	g_mapVisible[packed].timeout = g_timerGame + Tools_AdjustToGameSpeed(10 * 60, 0x0000, 0xFFFF, true);

	if (t->isUnveiled && Sprite_IsUnveiled(t->overlaySpriteID)) return false;
	t->isUnveiled = true;

	u = Unit_Get_ByPackedTile(packed);
	if (u != NULL) Unit_HouseUnitCount_Add(u, houseID);

	s = Structure_Get_ByPackedTile(packed);
	if (s != NULL) {
		for (enum HouseType ally = HOUSE_HARKONNEN; ally < HOUSE_MAX; ally++) {
			if (House_AreAllied(houseID, ally))
				s->o.seenByHouses |= (1 << ally);
		}
	}

	Map_UnveilTile_Neighbour(packed);
	Map_UnveilTile_Neighbour(packed + 1);
	Map_UnveilTile_Neighbour(packed - 1);
	Map_UnveilTile_Neighbour(packed - 64);
	Map_UnveilTile_Neighbour(packed + 64);

	return true;
}

void
Map_RefreshTile(uint16 packed)
{
	if (Tile_IsOutOfMap(packed)) return;

	Tile *t = &g_map[packed];

	if (t->isUnveiled)
		g_mapVisible[packed].timeout = g_timerGame + Tools_AdjustToGameSpeed(10 * 60, 0x0000, 0xFFFF, true);
}

void
Map_ResetFogOfWar(void)
{
	memset(g_mapVisible, 0, sizeof(g_mapVisible));

	for (uint16 packed = 0; packed < MAP_SIZE_MAX * MAP_SIZE_MAX; packed++) {
		g_mapVisible[packed].fogOverlayBits = 0xF;
	}
}

void
Map_UpdateFogOfWar(void)
{
	if (enhancement_fog_of_war) {
		for (uint16 packed = 65; packed < MAP_SIZE_MAX * MAP_SIZE_MAX - 65; packed++) {
			const Tile *t = &g_map[packed];
			FogOfWarTile *f = &g_mapVisible[packed];

			if (!t->isUnveiled || f->timeout <= g_timerGame) {
				f->fogOverlayBits = 0xF;
			}
			else {
				f->groundSpriteID = t->groundSpriteID;

				if (!(g_veiledSpriteID - 16 <= t->overlaySpriteID && t->overlaySpriteID <= g_veiledSpriteID))
					f->overlaySpriteID = t->overlaySpriteID;

				f->houseID = t->houseID;
				f->hasStructure = t->hasStructure;
				f->fogOverlayBits = 0;

				if (g_mapVisible[packed - 64].timeout <= g_timerGame) f->fogOverlayBits |= 0x1;
				if (g_mapVisible[packed +  1].timeout <= g_timerGame) f->fogOverlayBits |= 0x2;
				if (g_mapVisible[packed + 64].timeout <= g_timerGame) f->fogOverlayBits |= 0x4;
				if (g_mapVisible[packed -  1].timeout <= g_timerGame) f->fogOverlayBits |= 0x8;
			}
		}
	}
	else {
		for (uint16 packed = 65; packed < MAP_SIZE_MAX * MAP_SIZE_MAX - 65; packed++) {
			const Tile *t = &g_map[packed];
			FogOfWarTile *f = &g_mapVisible[packed];

			f->groundSpriteID = t->groundSpriteID;

			if (!(g_veiledSpriteID - 16 <= t->overlaySpriteID && t->overlaySpriteID <= g_veiledSpriteID))
				f->overlaySpriteID = t->overlaySpriteID;

			f->houseID = t->houseID;
			f->hasStructure = t->hasStructure;
			f->fogOverlayBits = (t->isUnveiled ? 0x0 : 0xF);
		}
	}
}

/**
 * Add spice on the given tile.
 * @param packed The tile.
 */
static void Map_AddSpiceOnTile(uint16 packed)
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

					if (Tile_IsOutOfMap(packed2)) continue;

					t2 = &g_map[packed2];

					if (!g_table_landscapeInfo[t2->groundSpriteID].canBecomeSpice) {
						t->groundSpriteID = LST_SPICE;
						continue;
					}

					if (t2->groundSpriteID != LST_THICK_SPICE) t2->groundSpriteID = LST_SPICE;
				}
			}
			return;
		}

		default:
			if (g_table_landscapeInfo[t->groundSpriteID].canBecomeSpice) t->groundSpriteID = LST_SPICE;
			return;
	}
}

static const uint16 _offsetTable[2][21][4] = {
	{
		{0, 0, 4, 0}, {4, 0, 4, 4}, {0, 0, 0, 4}, {0, 4, 4, 4}, {0, 0, 0, 2},
		{0, 2, 0, 4}, {0, 0, 2, 0}, {2, 0, 4, 0}, {4, 0, 4, 2}, {4, 2, 4, 4},
		{0, 4, 2, 4}, {2, 4, 4, 4}, {0, 0, 4, 4}, {2, 0, 2, 2}, {0, 0, 2, 2},
		{4, 0, 2, 2}, {0, 2, 2, 2}, {2, 2, 4, 2}, {2, 2, 0, 4}, {2, 2, 4, 4},
		{2, 2, 2, 4},
	},
	{
		{0, 0, 4, 0}, {4, 0, 4, 4}, {0, 0, 0, 4}, {0, 4, 4, 4}, {0, 0, 0, 2},
		{0, 2, 0, 4}, {0, 0, 2, 0}, {2, 0, 4, 0}, {4, 0, 4, 2}, {4, 2, 4, 4},
		{0, 4, 2, 4}, {2, 4, 4, 4}, {4, 0, 0, 4}, {2, 0, 2, 2}, {0, 0, 2, 2},
		{4, 0, 2, 2}, {0, 2, 2, 2}, {2, 2, 4, 2}, {2, 2, 0, 4}, {2, 2, 4, 4},
		{2, 2, 2, 4},
	},
};


/**
 * Creates the landscape using the given seed.
 * @param seed The seed.
 */
void Map_CreateLandscape(uint32 seed)
{
	static const int8 around[] = {0, -1, 1, -16, 16, -17, 17, -15, 15, -2, 2, -32, 32, -4, 4, -64, 64, -30, 30, -34, 34};

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
		if (memory[i] > 0xA) memory[i] = 0xA;
	}

	i = (Tools_Random_256() & 0xF) + 1;
	while (i-- != 0) {
		int16 base = Tools_Random_256();

		for (j = 0; j < lengthof(around); j++) {
			int16 index = min(max(0, base + around[j]), 272);

			memory[index] = (memory[index] + (Tools_Random_256() & 0xF)) & 0xF;
		}
	}

	i = (Tools_Random_256() & 0x3) + 1;
	while (i-- != 0) {
		int16 base = Tools_Random_256();

		for (j = 0; j < lengthof(around); j++) {
			int16 index = min(max(0, base + around[j]), 272);

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
				const uint16 *offsets = _offsetTable[(i + 1) % 2][k];
				uint16 packed1;
				uint16 packed2;
				uint16 packed;
				uint16 sprite2;

				packed1 = Tile_PackXY(i * 4 + offsets[0], j * 4 + offsets[1]);
				packed2 = Tile_PackXY(i * 4 + offsets[2], j * 4 + offsets[3]);
				packed = (packed1 + packed2) / 2;

				if (Tile_IsOutOfMap(packed)) continue;

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

		for (i = 0; i < 64; i++) currentRow[i] = t[i].groundSpriteID;

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
	if (spriteID1 < 0x8) spriteID1 = 0x8;
	if (spriteID1 > 0xC) spriteID1 = 0xC;

	spriteID2 = (Tools_Random_256() & 0x3) - 1;
	if (spriteID2 > spriteID1 - 3) spriteID2 = spriteID1 - 3;

	for (i = 0; i < 4096; i++) {
		uint16 spriteID = g_map[i].groundSpriteID;

		if (spriteID > spriteID1 + 4) {
			spriteID = LST_ENTIRELY_MOUNTAIN;
		} else if (spriteID >= spriteID1) {
			spriteID = LST_ENTIRELY_ROCK;
		} else if (spriteID <= spriteID2) {
			spriteID = LST_ENTIRELY_DUNE;
		} else {
			spriteID = LST_NORMAL_SAND;
		}

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

			if (g_table_landscapeInfo[g_map[packed].groundSpriteID].canBecomeSpice) break;
		}

		tile = Tile_UnpackTile(packed);

		j = Tools_Random_256() & 0x1F;
		while (j-- != 0) {
			while (true) {
				packed = Tile_PackTile(Tile_MoveByRandom(tile, Tools_Random_256() & 0x3F, true));

				if (!Tile_IsOutOfMap(packed)) break;
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
				default: break;
			}

			t[i].groundSpriteID = spriteID;
		}
	}

	/* Finalise the tiles with the real sprites. */
	iconMap = &g_iconMap[g_iconMap[ICM_ICONGROUP_LANDSCAPE]];

	for (i = 0; i < 4096; i++) {
		Tile *t = &g_map[i];

		t->groundSpriteID  = iconMap[t->groundSpriteID];
		t->overlaySpriteID = g_veiledSpriteID;
		t->houseID         = HOUSE_HARKONNEN;
		t->isUnveiled      = false;
		t->hasUnit         = false;
		t->hasStructure    = false;
		t->hasAnimation    = false;
		t->hasExplosion  = false;
		t->index           = 0;
	}

	for (i = 0; i < 4096; i++) g_mapSpriteID[i] = g_map[i].groundSpriteID;
}
