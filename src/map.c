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
#include "enhancement.h"
#include "explosion.h"
#include "gfx.h"
#include "gui/gui.h"
#include "gui/widget.h"
#include "house.h"
#include "net/server.h"
#include "newui/actionpanel.h"
#include "newui/menubar.h"
#include "opendune.h"
#include "pool/pool.h"
#include "pool/pool_house.h"
#include "pool/pool_structure.h"
#include "pool/pool_unit.h"
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
	} else {
		tilex = 0;
		g_viewport_scrollOffsetX = x;
	}

	if (y >= 0) {
		tiley = y / TILE_SIZE;
		g_viewport_scrollOffsetY = y % TILE_SIZE;
	} else {
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
		g_selectionState = Structure_IsValidBuildLocation(g_playerHouseID, packed, g_structureActiveType);
		g_selectionPosition = packed;
		return;
	}

	if ((packed != 0xFFFF && g_mapVisible[packed].fogSpriteID != g_veiledSpriteID) || g_debugScenario) {
		Structure *s;

		s = Structure_Get_ByPackedTile(packed);
		if (s != NULL) {
			const StructureInfo *si = &g_table_structureInfo[s->o.type];

			GUI_DisplayHint(s->o.houseID, si->o.hintStringID, si->o.spriteID);

			packed = Tile_PackTile(s->o.position);

			Map_SetSelectionSize(si->layout);

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

uint16 Map_Clamp_Packed(uint16 position)
{
	const MapInfo *mapInfo;

	if ((position & 0xC000) != 0) return 0;

	uint16 x = Tile_GetPackedX(position);
	uint16 y = Tile_GetPackedY(position);

	mapInfo = &g_mapInfos[g_scenario.mapScale];

	if (mapInfo->minX > x) x = mapInfo->minX;
	if ( (mapInfo->minX + mapInfo->sizeX - 1) < x) x = mapInfo->minX + mapInfo->sizeX - 1;
	if (mapInfo->minY > y) y = mapInfo->minY;
	if ( (mapInfo->minY + mapInfo->sizeY - 1) < y) y = mapInfo->minY + mapInfo->sizeY - 1;

	return Tile_PackXY(x, y);
}

bool
Map_IsUnveiledToHouse(enum HouseType houseID, uint16 packed)
{
	return (g_mapVisible[packed].isUnveiled & (1 << houseID));
}

bool
Map_IsPositionUnveiled(enum HouseType houseID, uint16 packed)
{
	if (g_debugScenario)
		return true;

	if (!House_IsHuman(houseID))
		return true;

	const enum HouseFlag houseIDBit = (1 << houseID);

	return (65 <= packed && packed < MAP_SIZE_MAX * MAP_SIZE_MAX - 65)
		&& (g_mapVisible[packed].isUnveiled & houseIDBit)
		&& (g_mapVisible[packed - 1].isUnveiled & houseIDBit)
		&& (g_mapVisible[packed + 1].isUnveiled & houseIDBit)
		&& (g_mapVisible[packed - MAP_SIZE_MAX].isUnveiled & houseIDBit)
		&& (g_mapVisible[packed + MAP_SIZE_MAX].isUnveiled & houseIDBit);
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

enum HouseFlag
Map_FindHousesInRadius(tile32 tile, int radius)
{
	const uint16 origin = Tile_PackTile(tile);
	const uint16 cx = Tile_GetPackedX(origin);
	const uint16 cy = Tile_GetPackedY(origin);
	enum HouseFlag houses = 0;

	for (int y = cy - radius; y <= cy + radius; y++) {
		if (!Map_InRangeY(y))
			continue;

		for (int x = cx - radius; x <= cx + radius; x++) {
			if (!Map_InRangeX(x))
				continue;

			const uint16 packed = Tile_PackXY(x, y);
			if (Tile_GetDistancePacked(origin, packed) > radius)
				continue;

			Unit *u;
			enum HouseType houseID = HOUSE_INVALID;
			if (g_map[packed].hasStructure) {
				houseID = g_map[packed].houseID;
			} else if (g_map[packed].hasUnit && (u = Unit_Get_ByPackedTile(packed)) != NULL) {
				const UnitInfo *ui = &g_table_unitInfo[u->o.type];
				if (!ui->flags.isBullet
						&&  u->o.flags.s.used
						&&  u->o.flags.s.allocated
						&& !u->o.flags.s.isNotOnMap
						&& !u->o.flags.s.inTransport)
					houseID = Unit_GetHouseID(u);
			}

			if ((houseID != HOUSE_INVALID) && !(houses & (1 << houseID)))
				houses |= (1 << houseID);
		}
	}

	return houses;
}

static bool Map_UpdateWall(uint16 packed)
{
	Tile *t;

	if (Map_GetLandscapeType(packed) != LST_WALL) return false;

	t = &g_map[packed];

	t->groundSpriteID = g_mapSpriteID[packed] & 0x1FF;
	t->overlaySpriteID = g_wallSpriteID;

	Structure_ConnectWall(packed, true);

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
	const Unit *unit_origin = Tools_Index_GetUnit(unitOriginEncoded);
	uint16 dist_adjust;
	uint16 origin = UNIT_INVALID;
	if (unit_origin != NULL) origin=unit_origin->o.type;

	if (!s_debugNoExplosionDamage && hitpoints != 0) {
		PoolFindStruct find;

		for (Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
				u != NULL;
				u = Unit_FindNext(&find)) {
			const UnitInfo *ui = &g_table_unitInfo[u->o.type];
			uint16 distance;
			Team *t;
			Unit *us;
			Unit *attack;

			distance = Tile_GetDistance(position, u->o.position) >> 4;
			if (distance >= reactionDistance) continue;

			uint16 hp_adjust = hitpoints;
			dist_adjust=distance >> 2;
			
			if (!(u->o.type == UNIT_SANDWORM && type == EXPLOSION_SANDWORM_SWALLOW) && u->o.type != UNIT_FRIGATE) {
			
				/* Logic for enhancement_attack_dir_consistency */
				if (enhancement_attack_dir_consistency == true || g_campaign_selected == CAMPAIGNID_MULTIPLAYER)
				{
					if (origin == UNIT_INFANTRY     ||
						origin == UNIT_SOLDIER      ||
						origin == UNIT_TANK         ||
						origin == UNIT_SIEGE_TANK   ||
						origin == UNIT_DEVASTATOR   ||
						origin == UNIT_TRIKE        ||
						origin == UNIT_RAIDER_TRIKE ||
						origin == UNIT_QUAD)
						/* Gun turret identifies as UNIT_INVALID */
					{
						if ((Tile_Center(position).x==Tile_Center(u->o.position).x) && (Tile_Center(position).y==Tile_Center(u->o.position).y))
						{
							if (hitpoints>10) {hp_adjust-=6;} /* Tanks */
							if (hitpoints> 5) {hp_adjust-=1;} /* Quads, Tanks again */
							if (hitpoints> 4) {hp_adjust-=2;} /* Trikes, Quads, Tanks again */
							dist_adjust = 0;
						}
					}
				}
			
				Unit_Damage(u, hp_adjust >> dist_adjust, 0);
			}

			if (House_IsHuman(u->o.houseID)) continue;

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
					Unit_Server_SetAction(u, ACTION_HUNT);
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
					if (u->actionID != ACTION_MOVE)
						Unit_Server_SetAction(u, ACTION_MOVE);
					u->targetMove = unitOriginEncoded;
					continue;
				}
			}

			if (ui->bulletType == UNIT_INVALID) continue;

			if (u->actionID == ACTION_GUARD && u->o.flags.s.byScenario) {
				Unit_Server_SetAction(u, ACTION_HUNT);
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

	const Unit *u = Tools_Index_GetUnit(unitOriginEncoded);
	Explosion_Start(type, position, (u != NULL) ? Unit_GetHouseID(u) : HOUSE_HARKONNEN);
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

	Explosion_Start(type, position, enhancement_nonordos_deviation ? houseID : HOUSE_ORDOS);

	for (Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {
		if (Tile_GetDistance(position, u->o.position) / 16 >= radius) continue;

		Unit_Deviate(u, 0, houseID);
	}
}

/**
 * Perform a bloom explosion, filling the area with spice.
 * @param packed Center position.
 * @param houseID %House causing the explosion.
 */
void Map_Bloom_ExplodeSpice(uint16 packed, enum HouseFlag houses)
{
	if (g_validateStrictIfZero == 0) {
		Unit_Remove(Unit_Get_ByPackedTile(packed));
		g_map[packed].groundSpriteID = g_mapSpriteID[packed] & 0x1FF;
		Map_MakeExplosion(EXPLOSION_SPICE_BLOOM_TREMOR, Tile_UnpackTile(packed), 0, 0);
	}

	Server_Send_PlayVoiceAtTile(houses, VOICE_SPICE_BLOOM_LOCATED, packed);

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
	uint16 type;
	uint16 spriteID;

	packed &= 0xFFF;
	type = Map_GetLandscapeType(packed);
	spriteID = 0;

	if (type == LST_SPICE || type == LST_THICK_SPICE) {
		uint8 i;

		for (i = 0; i < 4; i++) {
			const uint16 curPacked = packed + g_table_mapDiff[i];
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

	enemyHouseID = houseID;

	/* Find a house that belongs to the enemy */
	for (const Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {
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

			if (u != NULL)
				Unit_Server_SetAction(u, ACTION_HUNT);
			break;
		}

		case 3: {
			tile32 position = Tile_UnpackTile(packed);
			Unit *u;

			position = Tile_MoveByRandom(position, 16, true);

			/* ENHANCEMENT -- Dune2 inverted houseID and typeID arguments. */
			u = Unit_Create(UNIT_INDEX_INVALID, UNIT_INFANTRY, enemyHouseID, position, Tools_Random_256());

			if (u != NULL)
				Unit_Server_SetAction(u, ACTION_HUNT);
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
uint16
Map_Server_FindLocationTile(uint16 locationID, enum HouseType houseID)
{
	static const int16 mapBase[3] = {1, -2, -2};
	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];
	const uint16 mapOffset = mapBase[g_scenario.mapScale];

	uint16 ret = 0;

	if (locationID == 6) { /* Enemy Base */
		PoolFindStruct find;

		/* Find the house of an enemy */
		for (const Structure *s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
				s != NULL;
				s = Structure_FindNext(&find)) {
			if (Structure_SharesPoolElement(s->o.type))
				continue;

			if (s->o.houseID == houseID) continue;

			houseID = s->o.houseID;
			break;
		}
	}

	while (ret == 0) {
		switch (locationID) {
			case 0: /* North */
				ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX - 2), mapInfo->minY + mapOffset);
				break;

			case 1: /* East */
				ret = Tile_PackXY(mapInfo->minX + mapInfo->sizeX - mapOffset, mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY - 2));
				break;

			case 2: /* South */
				ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX - 2), mapInfo->minY + mapInfo->sizeY - mapOffset);
				break;

			case 3: /* West */
				ret = Tile_PackXY(mapInfo->minX + mapOffset, mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY - 2));
				break;

			case 4: /* Air */
			case 5: /* Visible -- Dune II scenarios don't use this. */
				ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX), mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY));
				break;

			case 6: /* Enemy Base */
			case 7: { /* Home Base */
				PoolFindStruct find;
				const Structure *s = Structure_FindFirst(&find, houseID, STRUCTURE_INVALID);

				if (s != NULL) {
					ret = Tile_PackTile(Tile_MoveByRandom(s->o.position, 120, true));
				} else {
					const Unit *u = Unit_FindFirst(&find, houseID, UNIT_INVALID);
					if (u != NULL) {
						ret = Tile_PackTile(Tile_MoveByRandom(u->o.position, 120, true));
					} else {
						ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(0, mapInfo->sizeX), mapInfo->minY + Tools_RandomLCG_Range(0, mapInfo->sizeY));
					}
				}
				break;
			}
			case 8: //Fremen
				ret = Tile_PackXY(mapInfo->minX + Tools_RandomLCG_Range(2, mapInfo->sizeX - 3), mapInfo->minY + Tools_RandomLCG_Range(2, mapInfo->sizeY - 3));
				break;

			default: return 0;
		}

		if (locationID >= 4 && houseID != HOUSE_INVALID) {
			if (House_IsHuman(houseID) && !Map_IsValidPosition(ret))
				ret = 0;
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

	if (radius == 0 || function < 2 || (position.x == 0 && position.y == 0))
		return;

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
uint16 Map_SearchSpice(uint16 packed, uint16 radius, enum HouseType visibleToHouseID)
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
			if (visibleToHouseID < HOUSE_NEUTRAL && !Map_IsUnveiledToHouse(g_playerHouseID, curPacked)) continue;

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
extern void Map_SelectNext(bool getNext);
#endif

static int64_t
Map_GetUnveilTimeout(enum TileUnveilCause cause)
{
	const int duration
		= (cause == UNVEILCAUSE_EXPLOSION || cause == UNVEILCAUSE_SHORT)
		? 2 : 10;

	return g_timerGame + Tools_AdjustToGameSpeed(duration * 60, 0, 0xFFFF, true);
}

/**
 * After unveiling, check neighbour tiles. This function handles one neighbour.
 * @param packed The neighbour tile of an unveiled tile.
 */
static void
Map_UnveilTile_Neighbour(enum HouseType houseID, uint16 packed)
{
	if (!Map_InRangePacked(packed)) return;

	int bits;

	if (Map_IsUnveiledToHouse(houseID, packed)) {
		bits = 0;

		for (int i = 0; i < 4; i++) {
			const uint16 neighbour = packed + g_table_mapDiff[i];

			if (Tile_IsOutOfMap(neighbour)
					|| !Map_IsUnveiledToHouse(houseID, neighbour)) {
				bits |= (1 << i);
			}
		}

		if (bits != 0xF) {
			Unit *u = Unit_Get_ByPackedTile(packed);

			if (u != NULL)
				Unit_HouseUnitCount_Add(u, houseID);
		}
	} else {
		bits = 0xF;
	}

	if (houseID == g_playerHouseID) {
		g_mapVisible[packed].fogSpriteID
			= (bits != 0)
			? g_iconMap[g_iconMap[ICM_ICONGROUP_FOG_OF_WAR] + bits] : 0;
	}
}

void
Map_UnveilTile(enum HouseType houseID, enum TileUnveilCause cause,
		uint16 packed)
{
	Structure *s;
	Unit *u;

	if (Tile_IsOutOfMap(packed))
		return;

	FogOfWarTile *f = &g_mapVisible[packed];

	f->cause[houseID] = max(f->cause[houseID], cause);
	f->timeout[houseID] = Map_GetUnveilTimeout(cause);

	u = Unit_Get_ByPackedTile(packed);
	if (u != NULL && (House_IsHuman(houseID) || u->o.type != UNIT_SANDWORM)) Unit_HouseUnitCount_Add(u, houseID);

	s = Structure_Get_ByPackedTile(packed);
	if (s != NULL)
		s->o.seenByHouses |= House_GetAllies(houseID);

	/* ENHANCEMENT -- Originally, this test came before
	 * Unit_Get_ByPackedTile and Structure_Get_ByPackedTile, causing
	 * objects spawned in scouted territory to be not known.
	 */
	if (Map_IsPositionUnveiled(houseID, packed))
		return;

	f->isUnveiled |= (1 << houseID);
	Map_UnveilTile_Neighbour(houseID, packed);
	Map_UnveilTile_Neighbour(houseID, packed + 1);
	Map_UnveilTile_Neighbour(houseID, packed - 1);
	Map_UnveilTile_Neighbour(houseID, packed - MAP_SIZE_MAX);
	Map_UnveilTile_Neighbour(houseID, packed + MAP_SIZE_MAX);
}

void
Map_RefreshTile(enum HouseType houseID, enum TileUnveilCause cause, uint16 packed)
{
	if (Tile_IsOutOfMap(packed))
		return;

	if (Map_IsUnveiledToHouse(houseID, packed)) {
		const int64_t timeout = Map_GetUnveilTimeout(cause);
		FogOfWarTile *f = &g_mapVisible[packed];

		if (f->timeout[houseID] < timeout) {
			f->cause[houseID] = max(f->cause[houseID], cause);
			f->timeout[houseID] = timeout;
		}
	}
}

void
Map_ResetFogOfWar(void)
{
	memset(g_mapVisible, 0, sizeof(g_mapVisible));

	for (uint16 packed = 0; packed < MAP_SIZE_MAX * MAP_SIZE_MAX; packed++) {
		FogOfWarTile *f = &g_mapVisible[packed];

		f->fogSpriteID = g_veiledSpriteID;
		f->fogOverlayBits = 0xF;
	}
}

void
Map_Client_UpdateFogOfWar(void)
{
	if (enhancement_fog_of_war) {
		for (uint16 packed = 65; packed < MAP_SIZE_MAX * MAP_SIZE_MAX - 65; packed++) {
			FogOfWarTile *f = &g_mapVisible[packed];

			if (!Map_IsUnveiledToHouse(g_playerHouseID, packed)
					|| (f->timeout[g_playerHouseID] <= g_timerGame)) {
				f->fogOverlayBits = 0xF;
			} else {
				const Tile *t = &g_map[packed];

				f->groundSpriteID = t->groundSpriteID;
				f->overlaySpriteID = t->overlaySpriteID;
				f->houseID = t->houseID;
				f->hasStructure = t->hasStructure;
				f->fogOverlayBits = 0;

				if (g_mapVisible[packed - 64].timeout[g_playerHouseID] <= g_timerGame) f->fogOverlayBits |= 0x1;
				if (g_mapVisible[packed +  1].timeout[g_playerHouseID] <= g_timerGame) f->fogOverlayBits |= 0x2;
				if (g_mapVisible[packed + 64].timeout[g_playerHouseID] <= g_timerGame) f->fogOverlayBits |= 0x4;
				if (g_mapVisible[packed -  1].timeout[g_playerHouseID] <= g_timerGame) f->fogOverlayBits |= 0x8;
			}
		}
	} else {
		for (uint16 packed = 65; packed < MAP_SIZE_MAX * MAP_SIZE_MAX - 65; packed++) {
			const Tile *t = &g_map[packed];
			FogOfWarTile *f = &g_mapVisible[packed];

			f->groundSpriteID = t->groundSpriteID;
			f->overlaySpriteID = t->overlaySpriteID;
			f->houseID = t->houseID;
			f->hasStructure = t->hasStructure;
			f->fogOverlayBits = Map_IsUnveiledToHouse(g_playerHouseID, packed) ? 0x0 : 0xF;
		}
	}
}
