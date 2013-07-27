/** @file src/structure.c %Structure handling routines. */

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "enum_string.h"
#include "types.h"
#include "os/math.h"
#include "os/strings.h"

#include "structure.h"

#include "ai.h"
#include "animation.h"
#include "audio/audio.h"
#include "enhancement.h"
#include "explosion.h"
#include "gfx.h"
#include "gui/gui.h"
#include "gui/widget.h"
#include "house.h"
#include "map.h"
#include "newui/actionpanel.h"
#include "opendune.h"
#include "pool/pool.h"
#include "pool/house.h"
#include "pool/structure.h"
#include "pool/team.h"
#include "pool/unit.h"
#include "scenario.h"
#include "sprites.h"
#include "string.h"
#include "team.h"
#include "tile.h"
#include "timer/timer.h"
#include "tools/coord.h"
#include "tools/encoded_index.h"
#include "tools/random_general.h"
#include "tools/random_lcg.h"
#include "tools/random_starport.h"
#include "unit.h"


Structure *g_structureActive = NULL;
uint16 g_structureActivePosition = 0;
uint16 g_structureActiveType = 0;

static bool s_debugInstantBuild = false; /*!< When non-zero, constructions are almost instant. */

uint16 g_structureIndex;

static bool Structure_SkipUpgradeLevel(const Structure *s, int level);

/**
 * Loop over all structures, preforming various of tasks.
 */
void GameLoop_Structure(void)
{
	PoolFindStruct find;
	bool tickDegrade   = false;
	bool tickStructure = false;
	bool tickScript    = false;
	bool tickPalace    = false;

	if (g_tickStructureDegrade <= g_timerGame && g_campaignID > 1) {
		tickDegrade = true;
		g_tickStructureDegrade = g_timerGame + Tools_AdjustToGameSpeed(10800, 5400, 21600, true);
	}

	if (g_tickStructureStructure <= g_timerGame || s_debugInstantBuild) {
		tickStructure = true;
		g_tickStructureStructure = g_timerGame + Tools_AdjustToGameSpeed(30, 15, 60, true);
	}

	if (g_tickStructureScript <= g_timerGame) {
		tickScript = true;
		g_tickStructureScript = g_timerGame + 5;
	}

	if (g_tickStructurePalace <= g_timerGame) {
		tickPalace = true;
		g_tickStructurePalace = g_timerGame + 60;
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	if (g_debugScenario) return;

	while (true) {
		const StructureInfo *si;
		const HouseInfo *hi;
		Structure *s;
		House *h;

		s = Structure_Find(&find);
		if (s == NULL) break;
		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;

		si = &g_table_structureInfo[s->o.type];
		h  = House_Get_ByIndex(s->o.houseID);
		hi = &g_table_houseInfo[h->index];

		g_scriptCurrentObject    = &s->o;
		g_scriptCurrentStructure = s;
		g_scriptCurrentUnit      = NULL;
		g_scriptCurrentTeam      = NULL;

		if (enhancement_fog_of_war)
			Structure_RemoveFog(s);

		if (tickPalace && s->o.type == STRUCTURE_PALACE) {
			if (s->countDown != 0) {
				s->countDown--;
			}

			/* Check if we have to fire the weapon for the AI immediately */
			if (s->countDown == 0 && !h->flags.human && h->flags.isAIActive) {
				Structure_ActivateSpecial(s);
			}
		}

		if (tickDegrade && s->o.flags.s.degrades && s->o.hitpoints > si->o.hitpoints / 2) {
			Structure_Damage(s, hi->degradingAmount, 0);
		}

		if (tickStructure) {
			if (s->o.flags.s.upgrading) {
				uint16 upgradeCost = si->o.buildCredits / 40;

				if (upgradeCost <= h->credits) {
					h->credits -= upgradeCost;

					if (s->upgradeTimeLeft > 5) {
						s->upgradeTimeLeft -= 5;
					} else {
						while (Structure_SkipUpgradeLevel(s, s->upgradeLevel + 1))
							s->upgradeLevel++;

						s->upgradeLevel++;
						s->o.flags.s.upgrading = false;

						/* ENHANCEMENT -- Resume production once upgrades are finished. */
						if (g_dune2_enhanced) s->o.flags.s.onHold = false;

						s->upgradeTimeLeft = Structure_IsUpgradable(s) ? 100 : 0;
						g_factoryWindowTotal = -1;
					}
				} else {
					s->o.flags.s.upgrading = false;

					/* ENHANCEMENT -- Resume production if upgrade stops from low credits. */
					if (g_dune2_enhanced) s->o.flags.s.onHold = false;
				}
			} else if (s->o.flags.s.repairing) {
				uint16 repairCost;

				switch (enhancement_repair_cost_formula) {
					default:
					case REPAIR_COST_v107:
					case REPAIR_COST_v107_HIGH_HP_FIX:
						repairCost = ((2 * 256 / si->o.hitpoints) * si->o.buildCredits + 128) / 256;

						/* ENHANCEMENT -- account for high hitpoints structures. */
						if (enhancement_repair_cost_formula == REPAIR_COST_v107_HIGH_HP_FIX && si->o.hitpoints > 512) {
							repairCost = (2 * 2 * si->o.buildCredits + si->o.hitpoints) / (2 * si->o.hitpoints);
						}
						break;

					case REPAIR_COST_v100:
						/* This seems to be the repair formula for v1.0, but only verified through testing. */
						repairCost = ((10 * 256 / si->o.hitpoints) * si->o.buildCredits + 128) / 256;
						break;

					case REPAIR_COST_OPENDUNE:
						/* OpenDUNE's repair cost calculation. */
						repairCost = 2 * si->o.buildCredits / si->o.hitpoints;
						break;
				}

				if (repairCost <= h->credits) {
					h->credits -= repairCost;

					/* AIs repair in early games slower than in later games */
					if (s->o.houseID == g_playerHouseID || g_campaignID >= 3) {
						s->o.hitpoints += 5;
					} else {
						s->o.hitpoints += 3;
					}

					if (s->o.hitpoints > si->o.hitpoints) {
						s->o.hitpoints = si->o.hitpoints;
						s->o.flags.s.repairing = false;
						s->o.flags.s.onHold = false;
					}
				} else {
					s->o.flags.s.repairing = false;

					/* ENHANCEMENT -- Resume production if repairing stops from low credits. */
					if (g_dune2_enhanced) s->o.flags.s.onHold = false;
				}
			} else {
				if (!s->o.flags.s.onHold && s->countDown != 0 && s->o.linkedID != 0xFF && s->state == STRUCTURE_STATE_BUSY && si->o.flags.factory) {
					ObjectInfo *oi;
					uint16 buildSpeed;
					uint16 buildCost;

					if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
						oi = &g_table_structureInfo[s->objectType].o;
					} else if (s->o.type == STRUCTURE_REPAIR) {
						oi = &g_table_unitInfo[Unit_Get_ByIndex(s->o.linkedID)->o.type].o;
					} else {
						oi = &g_table_unitInfo[s->objectType].o;
					}

					buildSpeed = 256;
					if (s->o.hitpoints < si->o.hitpoints) {
						buildSpeed = s->o.hitpoints * 256 / si->o.hitpoints;
					}

					if (g_playerHouseID != s->o.houseID) {
						if (AI_IsBrutalAI(s->o.houseID)) {
							/* For brutal AI, double production speed (except for ornithopters). */
							if (!(s->o.type == STRUCTURE_HIGH_TECH && s->objectType == UNIT_ORNITHOPTER))
								buildSpeed *= 2;
						}
						else if (buildSpeed > g_campaignID * 20 + 95) {
							/* For AIs, we slow down building speed in all but the last campaign */
							buildSpeed = g_campaignID * 20 + 95;
						}
					}

					buildCost = oi->buildCredits * 256 / oi->buildTime;

					/* For brutal AI, half production cost. */
					if (AI_IsBrutalAI(s->o.houseID)) {
						buildCost = buildSpeed * buildCost / (256 * 2);
					}
					else if (buildSpeed < 256) {
						buildCost = buildSpeed * buildCost / 256;
					}

					if (s->o.type == STRUCTURE_REPAIR && buildCost > 4) {
						buildCost /= 4;
					}

					buildCost += s->buildCostRemainder;

					if (buildCost / 256 <= h->credits) {
						s->buildCostRemainder = buildCost & 0xFF;
						h->credits -= buildCost / 256;

						if (buildSpeed < s->countDown) {
							s->countDown -= buildSpeed;
						} else {
							s->countDown = 0;
							s->buildCostRemainder = 0;

							Structure_SetState(s, STRUCTURE_STATE_READY);

							if (s->o.houseID == g_playerHouseID) {
								if (s->o.type != STRUCTURE_BARRACKS && s->o.type != STRUCTURE_WOR_TROOPER) {
									uint16 stringID = STR_IS_COMPLETED_AND_AWAITING_ORDERS;
									if (s->o.type == STRUCTURE_HIGH_TECH) stringID = STR_IS_COMPLETE;
									if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) stringID = STR_IS_COMPLETED_AND_READY_TO_PLACE;

									GUI_DisplayText("%s %s", 0, String_Get_ByIndex(oi->stringID_full), String_Get_ByIndex(stringID));

									Audio_PlayVoice(VOICE_CONSTRUCTION_COMPLETE);
								}
							} else if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
								/* An AI immediately places the structure when it is done building */
								Structure *ns;
								uint8 i;

								ns = Structure_Get_ByIndex(s->o.linkedID);
								s->o.linkedID = 0xFF;

								/* The AI places structures which are operational immediately */
								Structure_SetState(s, STRUCTURE_STATE_IDLE);

								/* Find the position to place the structure */
								for (i = 0; i < 5; i++) {
									if (ns->o.type != h->ai_structureRebuild[i][0]) continue;

									if (!Structure_Place(ns, h->ai_structureRebuild[i][1], h->index)) continue;

									h->ai_structureRebuild[i][0] = 0;
									h->ai_structureRebuild[i][1] = 0;
									break;
								}

								/* If the AI no longer had in memory where to store the structure, free it and forget about it */
								if (i == 5) {
									const StructureInfo *nsi = &g_table_structureInfo[ns->o.type];

									h->credits += nsi->o.buildCredits;

									Structure_Free(ns);
								}
							}
						}
					} else {
						/* Out of money means the building gets put on hold */
						if (s->o.houseID == g_playerHouseID) {
							if (!enhancement_construction_does_not_pause)
								s->o.flags.s.onHold = true;
							GUI_DisplayText(String_Get_ByIndex(STR_INSUFFICIENT_FUNDS_CONSTRUCTION_IS_HALTED), 0);
						}
					}
				}

				if (si->o.flags.factory) {
					bool start_next = false;

					if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
						if ((s->o.linkedID == 0xFF) && (g_selectionType != SELECTIONTYPE_PLACE))
							start_next = true;
					}
					else if (s->o.type == STRUCTURE_STARPORT) {
						start_next = false;
					}
					else if (s->state == STRUCTURE_STATE_IDLE) {
						start_next = true;
					}

					if (start_next) {
						uint16 object_type;

						object_type = BuildQueue_RemoveHead(&s->queue);
						while (object_type != 0xFFFF) {
							if (Structure_GetAvailable(s, object_type)) {
								if (Structure_BuildObject(s, object_type))
									break;
							}

							object_type = BuildQueue_RemoveHead(&s->queue);
						}

						if (object_type == 0xFFFF)
							s->state = STRUCTURE_STATE_IDLE;
					}
				}

				if (s->o.type == STRUCTURE_REPAIR) {
					if (!s->o.flags.s.onHold && s->countDown != 0 && s->o.linkedID != 0xFF) {
						const UnitInfo *ui;
						uint16 repairSpeed;
						uint16 repairCost;

						ui = &g_table_unitInfo[Unit_Get_ByIndex(s->o.linkedID)->o.type];

						repairSpeed = 256;
						if (s->o.hitpoints < si->o.hitpoints) {
							repairSpeed = s->o.hitpoints * 256 / si->o.hitpoints;
						}

						/* XXX -- This is highly unfair. Repairing becomes more expensive if your structure is more damaged */
						repairCost = 2 * ui->o.buildCredits / 256;

						if (repairCost < h->credits) {
							h->credits -= repairCost;

							if (repairSpeed < s->countDown) {
								s->countDown -= repairSpeed;
							} else {
								s->countDown = 0;

								Structure_SetState(s, STRUCTURE_STATE_READY);

								if (s->o.houseID == g_playerHouseID)
									Audio_PlayVoice(VOICE_HARKONNEN_VEHICLE_REPAIRED + g_playerHouseID);
							}
						}
					} else if (h->credits != 0) {
						/* Automaticly resume repairing when there is money again */
						s->o.flags.s.onHold = false;
					}
				}

				/* AI maintenance on structures */
				if (h->flags.isAIActive && s->o.flags.s.allocated && s->o.houseID != g_playerHouseID && h->credits != 0) {
					/* When structure is below 50% hitpoints, start repairing */
					if (s->o.hitpoints < si->o.hitpoints / 2) {
						Structure_SetRepairingState(s, 1, NULL);
					}

					/* If the structure is not doing something, but can build stuff, see if there is stuff to build */
					if (si->o.flags.factory && s->countDown == 0 && s->o.linkedID == 0xFF) {
						uint16 type = StructureAI_PickNextToBuild(s);

						if (type != 0xFFFF) Structure_BuildObject(s, type);
					}
				}
			}
		}

		if (tickScript) {
			if (s->o.script.delay != 0) {
				s->o.script.delay--;
			} else {
				if (Script_IsLoaded(&s->o.script)) {
					uint8 i;

					/* Run the script 3 times in a row */
					for (i = 0; i < 3; i++) {
						if (!Script_Run(&s->o.script)) break;
					}

					/* ENHANCEMENT -- Dune2 aborts all other structures if one gives a script error. This doesn't seem correct */
					if (!g_dune2_enhanced && i != 3) return;
				} else {
					Script_Reset(&s->o.script, s->o.script.scriptInfo);
					Script_Load(&s->o.script, s->o.type);
				}
			}
		}
	}
}

/**
 * Convert the name of a structure to the type value of that structure, or
 *  STRUCTURE_INVALID if not found.
 */
uint8 Structure_StringToType(const char *name)
{
	uint8 type;
	if (name == NULL) return STRUCTURE_INVALID;

	for (type = 0; type < STRUCTURE_MAX; type++) {
		if (strcasecmp(g_table_structureInfo[type].o.name, name) == 0) return type;
	}

	return STRUCTURE_INVALID;
}

/**
 * Create a new Structure.
 *
 * @param index The new index of the Structure, or STRUCTURE_INDEX_INVALID to assign one.
 * @param typeID The type of the new Structure.
 * @param houseID The House of the new Structure.
 * @param var0C An unknown parameter.
 * @return The new created Structure, or NULL if something failed.
 */
Structure *Structure_Create(uint16 index, uint8 typeID, uint8 houseID, uint16 position)
{
	const StructureInfo *si;
	Structure *s;

	if (houseID >= HOUSE_MAX) return NULL;
	if (typeID >= STRUCTURE_MAX) return NULL;

	si = &g_table_structureInfo[typeID];
	s = Structure_Allocate(index, typeID);
	if (s == NULL) return NULL;

	s->o.houseID            = houseID;
	s->creatorHouseID       = houseID;
	s->o.flags.s.isNotOnMap = true;
	s->o.position.x         = 0;
	s->o.position.y         = 0;
	s->o.linkedID           = 0xFF;
	s->state                = (g_debugScenario) ? STRUCTURE_STATE_IDLE : STRUCTURE_STATE_JUSTBUILT;
	s->squadID = SQUADID_INVALID;
	BuildQueue_Init(&s->queue);
	s->rallyPoint = 0xFFFF;
	s->factoryOffsetY = 0;

	if (typeID == STRUCTURE_TURRET) {
		s->rotationSpriteDiff = g_iconMap[g_iconMap[ICM_ICONGROUP_BASE_DEFENSE_TURRET] + 1];
	}
	if (typeID == STRUCTURE_ROCKET_TURRET) {
		s->rotationSpriteDiff = g_iconMap[g_iconMap[ICM_ICONGROUP_BASE_ROCKET_TURRET] + 1];
	}

	s->o.hitpoints  = si->o.hitpoints;
	s->hitpointsMax = si->o.hitpoints;

	/* Check if there is an upgrade available */
	if (si->o.flags.factory) {
		s->upgradeTimeLeft = Structure_IsUpgradable(s) ? 100 : 0;
	}

	s->objectType = 0xFFFF;

	Structure_BuildObject(s, 0xFFFE);

	s->countDown = 0;

#if 0
	/* AIs get the full upgrade immediately */
	if (houseID != g_playerHouseID) {
		while (true) {
			if (!Structure_IsUpgradable(s)) break;
			s->upgradeLevel++;
		}
		s->upgradeTimeLeft = 0;
	}
#endif

	if (position != 0xFFFF && !Structure_Place(s, position, houseID)) {
		Structure_Free(s);
		return NULL;
	}

	return s;
}

/**
 * Place a structure on the map.
 *
 * @param structure The structure to place on the map.
 * @param position The (packed) tile to place the struction on.
 * @return True if and only if the structure is placed on the map.
 */
bool Structure_Place(Structure *s, uint16 position, enum HouseType houseID)
{
	const StructureInfo *si;
	int16 validBuildLocation;

	if (s == NULL) return false;
	if (position == 0xFFFF) return false;

	si = &g_table_structureInfo[s->o.type];

	/* ENHANCEMENT -- If the construction yard was captured, we need to reset the house.
	 * This is also needed because concrete slabs and walls are shared, creating problems with saved games.
	 * Also, upgrade the factory when it is placed so the player doesn't get the AI's free upgrades.
	 */
	s->o.houseID = houseID;
	if (houseID != g_playerHouseID) {
		while (true) {
			if (!Structure_IsUpgradable(s)) break;
			s->upgradeLevel++;
		}
		s->upgradeTimeLeft = 0;
	}
	else {
		while (Structure_IsUpgradable(s) && Structure_SkipUpgradeLevel(s, s->upgradeLevel))
			s->upgradeLevel++;

		s->upgradeTimeLeft = Structure_IsUpgradable(s) ? 100 : 0;
	}

	switch (s->o.type) {
		case STRUCTURE_WALL: {
			Tile *t;

			if (Structure_IsValidBuildLocation(position, STRUCTURE_WALL) == 0) return false;

			t = &g_map[position];
			t->groundSpriteID = g_wallSpriteID + 1;
			/* ENHANCEMENT -- Dune2 wrongfully only removes the lower 2 bits, where the lower 3 bits are the owner. This is no longer visible. */
			t->houseID  = s->o.houseID;

			g_mapSpriteID[position] |= 0x8000;

			if (s->o.houseID == g_playerHouseID) Tile_RemoveFogInRadius(Tile_UnpackTile(position), 1);

			if (Map_IsPositionUnveiled(position)) t->overlaySpriteID = 0;

			Structure_ConnectWall(position, true);
			Structure_Free(s);

		} return true;

		case STRUCTURE_SLAB_1x1:
		case STRUCTURE_SLAB_2x2: {
			uint16 i, result;

			result = 0;

			for (i = 0; i < g_table_structure_layoutTileCount[si->layout]; i++) {
				uint16 curPos = position + g_table_structure_layoutTiles[si->layout][i];
				Tile *t = &g_map[curPos];

				if (Structure_IsValidBuildLocation(curPos, STRUCTURE_SLAB_1x1) == 0) continue;

				t->groundSpriteID = g_builtSlabSpriteID;
				t->houseID = s->o.houseID;

				g_mapSpriteID[curPos] |= 0x8000;

				if (s->o.houseID == g_playerHouseID) Tile_RemoveFogInRadius(Tile_UnpackTile(curPos), 1);

				if (Map_IsPositionUnveiled(curPos)) t->overlaySpriteID = 0;

				Map_Update(curPos, 0, false);

				result = 1;
			}

			/* XXX -- Dirt hack -- Parts of the 2x2 slab can be outside the building area, so by doing the same loop twice it will build for sure */
			if (s->o.type == STRUCTURE_SLAB_2x2) {
				for (i = 0; i < g_table_structure_layoutTileCount[si->layout]; i++) {
					uint16 curPos = position + g_table_structure_layoutTiles[si->layout][i];
					Tile *t = &g_map[curPos];

					if (Structure_IsValidBuildLocation(curPos, STRUCTURE_SLAB_1x1) == 0) continue;

					t->groundSpriteID = g_builtSlabSpriteID;
					t->houseID = s->o.houseID;

					g_mapSpriteID[curPos] |= 0x8000;

					if (s->o.houseID == g_playerHouseID) {
						Tile_RemoveFogInRadius(Tile_UnpackTile(curPos), 1);
						t->overlaySpriteID = 0;
					}

					Map_Update(curPos, 0, false);

					result = 1;
				}
			}

			if (result == 0) return false;

			Structure_Free(s);
		} return true;
	}

	/* ENHANCEMENT -- Dune 2 AI disregards tile occupancy altogether.
	 * This prevents the AI building structures on top of units and structures.
	 */
	if (enhancement_ai_respects_structure_placement && (s->o.houseID != g_playerHouseID)) {
		validBuildLocation = Structure_IsValidBuildLandscape(position, s->o.type);
	}
	else {
		validBuildLocation = Structure_IsValidBuildLocation(position, s->o.type);
	}

	if (validBuildLocation == 0 && !g_debugScenario && g_validateStrictIfZero == 0) return false;

	/* ENHACEMENT -- In Dune2, it only removes the fog around the top-left tile of a structure, leaving for big structures the right in the fog. */
	if (!g_dune2_enhanced && s->o.houseID == g_playerHouseID) Tile_RemoveFogInRadius(Tile_UnpackTile(position), 2);

	s->o.seenByHouses |= 1 << s->o.houseID;
	if (House_AreAllied(s->o.houseID, g_playerHouseID)) s->o.seenByHouses |= 0xFF;

	s->o.flags.s.isNotOnMap = false;

	s->o.position = Tile_UnpackTile(position);
	s->o.position.x &= 0xFF00;
	s->o.position.y &= 0xFF00;

	s->rotationSpriteDiff = 0;
	s->o.hitpoints  = si->o.hitpoints;
	s->hitpointsMax = si->o.hitpoints;

	/* If the return value is negative, there are tiles without slab. This gives a penalty to the hitpoints. */
	if (validBuildLocation < 0) {
		uint16 tilesWithoutSlab = -(int16)validBuildLocation;
		uint16 structureTileCount = g_table_structure_layoutTileCount[si->layout];

		s->o.hitpoints -= (si->o.hitpoints / 2) * tilesWithoutSlab / structureTileCount;

		s->o.flags.s.degrades = true;
	} else {
		if (!enhancement_structures_on_concrete_do_not_degrade) {
			s->o.flags.s.degrades = true;
		}
	}

	Script_Reset(&s->o.script, g_scriptStructure);

	s->o.script.variables[0] = 0;
	s->o.script.variables[4] = 0;

	/* XXX -- Weird .. if 'position' enters with 0xFFFF it is returned immediately .. how can this ever NOT happen? */
	if (position != 0xFFFF) {
		s->o.script.delay = 0;
		Script_Reset(&s->o.script, s->o.script.scriptInfo);
		Script_Load(&s->o.script, s->o.type);
	}

	{
		uint16 i;

		for (i = 0; i < g_table_structure_layoutTileCount[si->layout]; i++) {
			uint16 curPos = position + g_table_structure_layoutTiles[si->layout][i];
			Unit *u;

			u = Unit_Get_ByPackedTile(curPos);

			Unit_Remove(u);

			/* ENHACEMENT -- In Dune2, it only removes the fog around the top-left tile of a structure, leaving for big structures the right in the fog. */
			if (g_dune2_enhanced && s->o.houseID == g_playerHouseID) Tile_RemoveFogInRadius(Tile_UnpackTile(curPos), 2);

		}
	}

	if (s->o.type == STRUCTURE_WINDTRAP) {
		House *h;

		h = House_Get_ByIndex(s->o.houseID);
		h->windtrapCount += 1;
	}

	/* ENHANCEMENT -- Calculate structures built before calculating power and credits.
	 * This prevents MCV starts from draining credits when deployed as
	 * otherwise the game sees no structures built.
	 */
	{
		House *h;
		h = House_Get_ByIndex(s->o.houseID);
		h->structuresBuilt = Structure_GetStructuresBuilt(h);
	}

	if (g_validateStrictIfZero == 0) {
		House *h;

		h = House_Get_ByIndex(s->o.houseID);
		House_CalculatePowerAndCredit(h);
	}

	Structure_UpdateMap(s);

#if 0
	{
		House *h;
		h = House_Get_ByIndex(s->o.houseID);
		h->structuresBuilt = Structure_GetStructuresBuilt(h);
	}
#endif

	return true;
}

/**
 * Calculate the power usage and production, and the credits storage.
 *
 * @param h The house to calculate the numbers for.
 */
void Structure_CalculateHitpointsMax(House *h)
{
	PoolFindStruct find;
	uint16 power = 0;

	if (h == NULL) return;

	if (h->index == g_playerHouseID) House_UpdateRadarState(h);

	if (h->powerUsage == 0) {
		power = 256;
	} else {
		power = min(h->powerProduction * 256 / h->powerUsage, 256);
	}

	find.houseID = h->index;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		const StructureInfo *si;
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) return;
		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;

		si = &g_table_structureInfo[s->o.type];

		s->hitpointsMax = si->o.hitpoints * power / 256;
		s->hitpointsMax = max(s->hitpointsMax, si->o.hitpoints / 2);

		if (s->hitpointsMax >= s->o.hitpoints) continue;
		Structure_Damage(s, 1, 0);
	}
}

/**
 * Set the state for the given structure.
 *
 * @param s The structure to set the state of.
 * @param state The new sate value.
 */
void Structure_SetState(Structure *s, int16 state)
{
	if (s == NULL) return;
	s->state = state;

	Structure_UpdateMap(s);
}

bool
Structure_SupportsRallyPoints(enum StructureType s)
{
	return ((s == STRUCTURE_LIGHT_VEHICLE) ||
	        (s == STRUCTURE_HEAVY_VEHICLE) ||
	        (s == STRUCTURE_WOR_TROOPER) ||
	        (s == STRUCTURE_BARRACKS) ||
	        (s == STRUCTURE_STARPORT) ||
	        (s == STRUCTURE_REFINERY) ||
	        (s == STRUCTURE_REPAIR));
}

void Structure_SetRallyPoint(Structure *s, uint16 packed)
{
	if (s->o.houseID != g_playerHouseID)
		return;

	const StructureInfo *si = &g_table_structureInfo[s->o.type];
	const uint8 tx = Tile_GetPackedX(packed);
	const uint8 ty = Tile_GetPackedY(packed);
	const uint8 x1 = Tile_GetPosX(s->o.position);
	const uint8 y1 = Tile_GetPosY(s->o.position);
	const uint8 x2 = x1 + g_table_structure_layoutSize[si->layout].width - 1;
	const uint8 y2 = y1 + g_table_structure_layoutSize[si->layout].height - 1;

	if ((x1 <= tx && tx <= x2) && (y1 <= ty && ty <= y2)) {
		s->rallyPoint = 0xFFFF;
	}
	else {
		s->rallyPoint = packed;
	}
}

/**
 * Get the structure on the given packed tile.
 *
 * @param packed The packed tile to get the structure from.
 * @return The structure.
 */
Structure *Structure_Get_ByPackedTile(uint16 packed)
{
	Tile *tile;

	if (Tile_IsOutOfMap(packed)) return NULL;

	tile = &g_map[packed];
	if (!tile->hasStructure) return NULL;
	return Structure_Get_ByIndex(tile->index - 1);
}

/**
 * Get a bitmask of all built structure types for the given House.
 *
 * @param h The house to get built structures for.
 * @return The bitmask.
 */
uint32 Structure_GetStructuresBuilt(House *h)
{
	PoolFindStruct find;
	uint32 result;

	if (h == NULL) return 0;

	result = 0;
	find.houseID = h->index;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	/* ENHANCEMENT -- recount windtraps after capture or loading old saved games. */
	if (enhancement_fix_typos) h->windtrapCount = 0;

	while (true) {
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;
		if (s->o.flags.s.isNotOnMap) continue;
		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;
		result |= 1 << s->o.type;

		if (enhancement_fix_typos && s->o.type == STRUCTURE_WINDTRAP) h->windtrapCount++;
	}

	return result;
}

/**
 * Checks if the given position is a valid location for the given structure type.
 *
 * @param position The (packed) tile to check.
 * @param type The structure type to check the position for.
 * @return 0 if the position is not valid, 1 if the position is valid and have enough slabs, <0 if the position is valid but miss some slabs.
 */
int16
Structure_IsValidBuildLandscape(uint16 position, enum StructureType type)
{
	const StructureInfo *si;
	const uint16 *layoutTile;
	uint8 i;
	uint16 neededSlabs;
	bool isValid;
	uint16 curPos;

	si = &g_table_structureInfo[type];
	layoutTile = g_table_structure_layoutTiles[si->layout];

	isValid = true;
	neededSlabs = 0;
	for (i = 0; i < g_table_structure_layoutTileCount[si->layout]; i++) {
		uint16 lst;

		curPos = position + layoutTile[i];

		lst = Map_GetLandscapeType(curPos);

		if (g_debugScenario) {
			if (!g_table_landscapeInfo[lst].isValidForStructure2) {
				isValid = false;
				break;
			}
		} else {
			if (!Map_IsValidPosition(curPos)) {
				isValid = false;
				break;
			}

			if (si->o.flags.notOnConcrete) {
				if (!g_table_landscapeInfo[lst].isValidForStructure2 && g_validateStrictIfZero == 0) {
					isValid = false;
					break;
				}
			} else {
				if (!g_table_landscapeInfo[lst].isValidForStructure && g_validateStrictIfZero == 0) {
					isValid = false;
					break;
				}
				if (lst != LST_CONCRETE_SLAB) neededSlabs++;
			}
		}

		if (Object_GetByPackedTile(curPos) != NULL) {
			isValid = false;
			break;
		}
	}

	if (!isValid) return 0;
	if (neededSlabs == 0) return 1;
	return -neededSlabs;
}

int16
Structure_IsValidBuildLocation(uint16 position, enum StructureType type)
{
	const StructureInfo *si;
	uint8 i;
	int16 retSlabs;
	bool isValid;
	uint16 curPos;

	si = &g_table_structureInfo[type];
	retSlabs = Structure_IsValidBuildLandscape(position, type);
	isValid = (retSlabs != 0);

	if (g_validateStrictIfZero == 0 && isValid && type != STRUCTURE_CONSTRUCTION_YARD && !g_debugScenario) {
		isValid = false;
		for (i = 0; i < 16; i++) {
			uint16 offset, lst;
			Structure *s;

			offset = g_table_structure_layoutTilesAround[si->layout][i];
			if (offset == 0) break;

			curPos = position + offset;
			s = Structure_Get_ByPackedTile(curPos);
			if (s != NULL) {
				if (s->o.houseID != g_playerHouseID) continue;
				isValid = true;
				break;
			}

			lst = Map_GetLandscapeType(curPos);
			if (lst != LST_CONCRETE_SLAB && lst != LST_WALL) continue;
			if (g_map[curPos].houseID != g_playerHouseID) continue;

			isValid = true;
			break;
		}
	}

	if (!isValid) return 0;
	return retSlabs;
}

/**
 * Activate the special weapon of a house.
 *
 * @param s The structure which launches the weapon. Has to be the Palace.
 */
void Structure_ActivateSpecial(Structure *s)
{
	House *h;

	if (s == NULL) return;
	if (s->o.type != STRUCTURE_PALACE) return;

	h = House_Get_ByIndex(s->o.houseID);
	if (!h->flags.used) return;

	const HouseInfo *hi = &g_table_houseInfo[s->o.houseID];
	switch (hi->specialWeapon) {
		case HOUSE_WEAPON_MISSILE: {
			Unit *u;
			tile32 position;

			position.x = 0xFFFF;
			position.y = 0xFFFF;

			g_validateStrictIfZero++;
			u = Unit_Create(UNIT_INDEX_INVALID, UNIT_MISSILE_HOUSE, s->o.houseID, position, Tools_Random_256());
			g_validateStrictIfZero--;

			g_unitHouseMissile = u;
			if (u == NULL) break;

			s->countDown = g_table_houseInfo[s->o.houseID].specialCountDown;

			if (!h->flags.human) {
				PoolFindStruct find;

				find.houseID = HOUSE_INVALID;
				find.type    = 0xFFFF;
				find.index   = 0xFFFF;

				/* For the AI, try to find the first structure which is not ours, and launch missile to there */
				while (true) {
					Structure *sf;

					sf = Structure_Find(&find);
					if (sf == NULL) break;
					if (sf->o.type == STRUCTURE_SLAB_1x1 || sf->o.type == STRUCTURE_SLAB_2x2 || sf->o.type == STRUCTURE_WALL) continue;

					if (House_AreAllied(s->o.houseID, sf->o.houseID)) continue;

					Unit_LaunchHouseMissile(s, Tile_PackTile(sf->o.position));

					return;
				}

				/* We failed to find a target, so remove the missile */
				Unit_Free(u);
				g_unitHouseMissile = NULL;

				return;
			}

			/* Give the user 7 seconds to select their target */
			g_houseMissileCountdown = 7;

			GUI_ChangeSelectionType(SELECTIONTYPE_TARGET);
		} break;

		case HOUSE_WEAPON_FREMEN: {
			uint16 location;
			uint16 i;

			/* Find a random location to appear */
			location = Map_FindLocationTile(4, HOUSE_INVALID);

			for (i = 0; i < 5; i++) {
				Unit *u;
				tile32 position;
				uint16 orientation;
				uint16 unitType;

				Tools_Random_256();

				position = Tile_UnpackTile(location);
				position = Tile_MoveByRandom(position, 32, true);

				orientation = Tools_RandomLCG_Range(0, 3);
				unitType = (orientation == 1) ? hi->superWeapon.fremen.unit25 : hi->superWeapon.fremen.unit75;

				g_validateStrictIfZero++;
				u = Unit_Create(UNIT_INDEX_INVALID, (uint8)unitType, hi->superWeapon.fremen.owner, position, (int8)orientation);
				g_validateStrictIfZero--;

				if (u == NULL) continue;

				Unit_SetAction(u, ACTION_HUNT);
			}

			s->countDown = g_table_houseInfo[s->o.houseID].specialCountDown;
		} break;

		case HOUSE_WEAPON_SABOTEUR: {
			Unit *u = NULL;
			uint16 position;

			/* Find a spot next to the structure */
			position = Structure_FindFreePosition(s, false);

			/* If there is no spot, reset countdown */
			if (position == 0) {
				/* ENHANCEMENT -- Do not reset the countdown to one as that causes an irritating blink. */
				if (!g_dune2_enhanced) s->countDown = 1;
			}
			else {
				g_validateStrictIfZero++;
				u = Unit_Create(UNIT_INDEX_INVALID, hi->superWeapon.saboteur.unit, hi->superWeapon.saboteur.owner, Tile_UnpackTile(position), Tools_Random_256());
				g_validateStrictIfZero--;
			}

			if (u != NULL) {
				s->countDown = g_table_houseInfo[s->o.houseID].specialCountDown;
				Unit_SetAction(u, ACTION_SABOTAGE);
			}
			else if (enhancement_play_additional_voices && s->o.houseID == g_playerHouseID) {
				Audio_PlaySound(EFFECT_ERROR_OCCURRED);
			}
		} break;

		default: break;
	}
}

/**
 * Remove the fog around a structure.
 *
 * @param s The Structure.
 */
void Structure_RemoveFog(Structure *s)
{
	const StructureInfo *si;
	tile32 position;

	if (s == NULL) return;

	/* ENHANCEMENT -- shared vision between allies.  Dune II didn't have allied structures. */
	if (!House_AreAllied(s->o.houseID, g_playerHouseID))
		return;

	si = &g_table_structureInfo[s->o.type];

	position = s->o.position;

	/* ENHANCEMENT -- Fog is removed around the top left corner instead of the center of a structure. */
	if (g_dune2_enhanced) {
		position.x += 256 * (g_table_structure_layoutSize[si->layout].width  - 1) / 2;
		position.y += 256 * (g_table_structure_layoutSize[si->layout].height - 1) / 2;
	}

	Tile_RemoveFogInRadius(position, si->o.fogUncoverRadius);
}

/**
 * Handles destroying of a structure.
 *
 * @param s The Structure.
 */
static void Structure_Destroy(Structure *s)
{
	const StructureInfo *si;
	uint8 linkedID;
	House *h;

	if (s == NULL) return;

	if (g_debugScenario) {
		Structure_Remove(s);
		return;
	}

	s->o.script.variables[0] = 1;
	s->o.flags.s.allocated = false;
	s->o.flags.s.repairing = false;
	s->o.script.delay = 0;

	Script_Reset(&s->o.script, g_scriptStructure);
	Script_Load(&s->o.script, s->o.type);

	Audio_PlaySoundAtTile(SOUND_STRUCTURE_DESTROYED, s->o.position);

	linkedID = s->o.linkedID;

	if (linkedID != 0xFF) {
		if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
			Structure_Destroy(Structure_Get_ByIndex(linkedID));
			s->o.linkedID = 0xFF;
		} else {
			while (linkedID != 0xFF) {
				Unit *u = Unit_Get_ByIndex(linkedID);

				linkedID = u->o.linkedID;

				Unit_Remove(u);
			}
		}
	}

	h = House_Get_ByIndex(s->o.houseID);
	si = &g_table_structureInfo[s->o.type];

	h->credits -= (h->creditsStorage == 0) ? h->credits : min(h->credits, (h->credits * 256 / h->creditsStorage) * si->creditsStorage / 256);

	if (s->o.houseID != g_playerHouseID) h->credits += si->o.buildCredits + (g_campaignID > 7 ? si->o.buildCredits / 2 : 0);

	if (s->o.type != STRUCTURE_WINDTRAP) return;

	h->windtrapCount--;
}

/**
 * Damage the structure, and bring the surrounding to an explosion if needed.
 *
 * @param s The structure to damage.
 * @param damage The damage to deal to the structure.
 * @param range The range in which an explosion should be possible.
 * @return True if and only if the structure is now destroyed.
 */
bool Structure_Damage(Structure *s, uint16 damage, uint16 range)
{
	const StructureInfo *si;

	if (s == NULL) return false;
	if (damage == 0) return false;
	if (s->o.script.variables[0] == 1) return false;

	si = &g_table_structureInfo[s->o.type];

	if (s->o.hitpoints >= damage) {
		s->o.hitpoints -= damage;
	} else {
		s->o.hitpoints = 0;
	}

	if (s->o.hitpoints == 0) {
		uint16 score;

		score = si->o.buildCredits / 100;
		if (score < 1) score = 1;

		if (House_AreAllied(g_playerHouseID, s->o.houseID)) {
			g_scenario.destroyedAllied++;
			g_scenario.score -= score;
		} else {
			g_scenario.destroyedEnemy++;
			g_scenario.score += score;
		}

		Structure_Destroy(s);

		if (House_AreAllied(g_playerHouseID, s->o.houseID)) {
			Audio_PlayVoice(VOICE_HARKONNEN_STRUCTURE_DESTROYED + s->o.houseID);
		} else {
			Audio_PlayVoice(VOICE_ENEMY_STRUCTURE_DESTROYED);
		}

		Structure_UntargetMe(s);
		return true;
	}

	if (range == 0) return false;

	Map_MakeExplosion(EXPLOSION_IMPACT_LARGE, Tile_AddTileDiff(s->o.position, g_table_structure_layoutTileDiff[si->layout]), 0, 0);
	return false;
}

static bool
Structure_UpgradeUnlocksNewUnit(const Structure *s, int level)
{
	const StructureInfo *si = &g_table_structureInfo[s->o.type];
	const uint8 creatorFlag = (1 << s->creatorHouseID);

	for (int i = 0; i < 8; i++) {
		if (si->buildableUnits[i] == UNIT_INVALID)
			continue;

		const enum UnitType u = si->buildableUnits[i];
		const UnitInfo *ui = &g_table_unitInfo[u];

		/* An upgrade is only possible if the original factory design
		 * supports it (creator) and the current owner has the technology.
		 */
		if ((ui->o.availableHouse & creatorFlag) && (level == ui->o.upgradeLevelRequired[s->o.houseID]))
			return true;
	}

	return false;
}

/**
 * Check wether the given structure is upgradable.
 *
 * @param s The Structure to check.
 * @return True if and only if the structure is upgradable.
 */
bool Structure_IsUpgradable(Structure *s)
{
	const StructureInfo *si;

	if (s == NULL) return false;

	si = &g_table_structureInfo[s->o.type];

	/* Use per-house upgrade levels: Harkonnen can never upgrade hi-tech. */
	/* if (s->o.houseID == HOUSE_HARKONNEN && s->o.type == STRUCTURE_HIGH_TECH) {} */

	/* Use per-house upgrade levels: Ordos siege tanks may come one level late. */
	/* if (s->o.houseID == HOUSE_ORDOS && s->o.type == STRUCTURE_HEAVY_VEHICLE && s->upgradeLevel == 1) {} */

	int next_upgrade_level = s->upgradeLevel;

	if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
		/* XXX: Too lazy to do this case. */
	}
	else {
		while (next_upgrade_level < 3) {
			if (Structure_UpgradeUnlocksNewUnit(s, next_upgrade_level + 1))
				break;

			next_upgrade_level++;
		}
	}

	if (si->upgradeCampaign[next_upgrade_level][s->o.houseID] != 0 &&
	    si->upgradeCampaign[next_upgrade_level][s->o.houseID] <= g_campaignID + 1) {
		House *h;

		if (s->o.type != STRUCTURE_CONSTRUCTION_YARD) return true;
		if (s->upgradeLevel != 1) return true;

		h = House_Get_ByIndex(s->o.houseID);
		if ((h->structuresBuilt & g_table_structureInfo[STRUCTURE_ROCKET_TURRET].o.structuresRequired) == g_table_structureInfo[STRUCTURE_ROCKET_TURRET].o.structuresRequired) return true;

		return false;
	}

	/* Use per-house upgrade levels: Harkonnen WOR one level earlier. */
	/* if (s->o.houseID == HOUSE_HARKONNEN && s->o.type == STRUCTURE_WOR_TROOPER && s->upgradeLevel == 0 && g_campaignID > 3) {} */
	return false;
}

static bool
Structure_SkipUpgradeLevel(const Structure *s, int level)
{
	const StructureInfo *si = &g_table_structureInfo[s->o.type];

	if ((!si->o.flags.factory) || (level >= 3))
		return false;

	if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
		/* XXX: Too lazy to do this case. */
		return false;
	}
	else {
#if 0
		/* Original behaviour: Harkonnen cannot produce trikes,
		 * so light factories begin upgraded.
		 */
		if (s->creatorHouseID == HOUSE_HARKONNEN && typeID == STRUCTURE_LIGHT_VEHICLE) {}

		/* Original behaviour: Ordos cannot produce launchers,
		 * so heavy factories skip an upgrade.
		 * upgradeLevel 0 -> 1 -> 3; not 0 -> 2 -> 3.
		 *
		 * ENHANCEMENT -- was s->o.houseID, but should be s->creatorHouseID.
		 * The original behaviour meant that if you captured an Ordos
		 * heavy factory on level 6, you could upgrade the factory twice.
		 * The second factory upgrade doesn't unlock any new units.
		 *
		 * Also, originally Ordos would unlock both launchers and
		 * siege tanks when upgrading enemy factories from level 1 due
		 * to the freebie upgrade.  This doesn't occur in the game
		 * since the AI always has full upgrades, but you can restore
		 * this behaviour by setting Ordos launcher's
		 * upgradeLevelRequired to 0.
		 */
		if (s->creatorHouseID == HOUSE_ORDOS && s->o.type == STRUCTURE_HEAVY_VEHICLE && s->upgradeLevel == 2) {}
#endif

		/* Generalised behaviour: the upgrade is free if it does not
		 * unlock any new tech.
		 */
		return !Structure_UpgradeUnlocksNewUnit(s, level);
	}
}

/**
 * Connect walls around the given position.
 *
 * @param position The packed position.
 * @param recurse Wether to recurse.
 * @return True if and only if a change happened.
 */
bool Structure_ConnectWall(uint16 position, bool recurse)
{
	static const int16 offset[] = { -64, 1, 64, -1 };
	static const uint8 wall[] = {
		 0,  3,  1,  2,  3,  3,  4,  5,  1,  6,  1,  7,  8,  9, 10, 11,
		 1, 12,  1, 19,  1, 16,  1, 31,  1, 28,  1, 52,  1, 45,  1, 59,
		 3,  3, 13, 20,  3,  3, 22, 32,  3,  3, 13, 53,  3,  3, 38, 60,
		 5,  6,  7, 21,  5,  6,  7, 33,  5,  6,  7, 54,  5,  6,  7, 61,
		 9,  9,  9,  9, 17, 17, 23, 34,  9,  9,  9,  9, 25, 46, 39, 62,
		11, 12, 11, 12, 13, 18, 13, 35, 11, 12, 11, 12, 13, 47, 13, 63,
		15, 15, 16, 16, 17, 17, 24, 36, 15, 15, 16, 16, 17, 17, 40, 64,
		19, 20, 21, 22, 23, 24, 25, 37, 19, 20, 21, 22, 23, 24, 25, 65,
		27, 27, 27, 27, 27, 27, 27, 27, 14, 29, 14, 55, 26, 48, 41, 66,
		29, 30, 29, 30, 29, 30, 29, 30, 31, 30, 31, 56, 31, 49, 31, 67,
		33, 33, 34, 34, 33, 33, 34, 34, 35, 35, 15, 57, 35, 35, 42, 68,
		37, 38, 39, 40, 37, 38, 39, 40, 41, 42, 43, 58, 41, 42, 43, 69,
		45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, 27, 50, 43, 70,
		49, 50, 49, 50, 51, 52, 51, 52, 53, 54, 53, 54, 55, 51, 55, 71,
		57, 57, 58, 58, 59, 59, 60, 60, 61, 61, 62, 62, 63, 63, 44, 72,
		65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 73
	};

	uint16 bits = 0;
	uint16 spriteID;
	bool isDestroyedWall;
	uint8 i;
	Tile *tile;

	isDestroyedWall = Map_GetLandscapeType(position) == LST_DESTROYED_WALL;

	for (i = 0; i < 4; i++) {
		uint16 curPos = position + offset[i];

		if (recurse && Map_GetLandscapeType(curPos) == LST_WALL) Structure_ConnectWall(curPos, false);

		if (isDestroyedWall) continue;

		switch (Map_GetLandscapeType(curPos)) {
			case LST_DESTROYED_WALL: bits |= (1 << (i + 4));
				/* FALL-THROUGH */
			case LST_WALL: bits |= (1 << i);
				/* FALL-THROUGH */
			default:  break;
		}
	}

	if (isDestroyedWall) return false;

	spriteID = g_wallSpriteID + wall[bits] + 1;

	tile = &g_map[position];
	if (tile->groundSpriteID == spriteID) return false;

	tile->groundSpriteID = spriteID;
	g_mapSpriteID[position] |= 0x8000;
	Map_Update(position, 0, false);

	return true;
}

/**
 * Get the unit linked to this structure, or NULL if there is no.
 * @param s The structure to get the linked unit from.
 * @return The linked unit, or NULL if there was none.
 */
Unit *Structure_GetLinkedUnit(Structure *s)
{
	if (s->o.linkedID == 0xFF) return NULL;
	return Unit_Get_ByIndex(s->o.linkedID);
}

/**
 * Untarget the given Structure.
 *
 * @param unit The Structure to untarget.
 */
void Structure_UntargetMe(Structure *s)
{
	PoolFindStruct find;
	uint16 encoded = Tools_Index_Encode(s->o.index, IT_STRUCTURE);

	Object_Script_Variable4_Clear(&s->o);

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		if (u->targetMove == encoded) u->targetMove = 0;
		if (u->targetAttack == encoded) u->targetAttack = 0;
		if (u->o.script.variables[4] == encoded) Object_Script_Variable4_Clear(&u->o);
	}

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Team *t;

		t = Team_Find(&find);
		if (t == NULL) break;

		if (t->target == encoded) t->target = 0;
	}
}

/**
 * Find a free spot for units next to a structure.
 * @param s Structure that needs a free spot.
 * @param checkForSpice Spot should be as close to spice as possible.
 * @return Position of the free spot, or \c 0 if no free spot available.
 */
uint16 Structure_FindFreePosition(Structure *s, bool checkForSpice)
{
	const StructureInfo *si;
	uint16 packed;
	uint16 spicePacked;  /* Position of the spice, or 0 if not used or if no spice. */
	uint16 bestPacked;
	uint16 bestDistance; /* If > 0, distance to the spice from bestPacked. */
	uint16 i, j;

	if (s == NULL) return 0;

	si = &g_table_structureInfo[s->o.type];
	packed = Tile_PackTile(Tile_Center(s->o.position));

	spicePacked = (checkForSpice) ? Map_SearchSpice(packed, 10) : 0;
	bestPacked = 0;
	bestDistance = 0;

	/* ENHANCEMENT -- Old code advanced i by 2, thus could run past
	 * the end of g_table_structure_layoutTilesAround.
	 */
	i = Tools_Random_256() & 0xF;
	for (j = 0; j < 16; j++, i = (i + 1) & 0xF) {
		uint16 offset;
		uint16 curPacked;
		uint16 type;
		Tile *t;

		offset = g_table_structure_layoutTilesAround[si->layout][i];
		if (offset == 0) continue;

		curPacked = packed + offset;
		if (!Map_IsValidPosition(curPacked)) continue;

		type = Map_GetLandscapeType(curPacked);
		if (type == LST_WALL || type == LST_ENTIRELY_MOUNTAIN || type == LST_PARTIAL_MOUNTAIN) continue;

		t = &g_map[curPacked];
		if (t->hasUnit || t->hasStructure) continue;

		if (!checkForSpice) return curPacked;

		if (bestDistance == 0 || Tile_GetDistancePacked(curPacked, spicePacked) < bestDistance) {
			bestPacked = curPacked;
			bestDistance = Tile_GetDistancePacked(curPacked, spicePacked);
		}
	}

	return bestPacked;
}

/**
 * Remove the structure from the map, free it, and clean up after it.
 * @param s The structure to remove.
 */
void Structure_Remove(Structure *s)
{
	const StructureInfo *si;
	uint16 packed;
	uint16 i;
	House *h;

	if (s == NULL) return;

	si = &g_table_structureInfo[s->o.type];
	packed = Tile_PackTile(s->o.position);

	for (i = 0; i < g_table_structure_layoutTileCount[si->layout]; i++) {
		Tile *t;
		uint16 curPacked = packed + g_table_structure_layoutTiles[si->layout][i];

		Animation_Stop_ByTile(curPacked);

		t = &g_map[curPacked];
		t->hasStructure = false;

		if (g_debugScenario) {
			t->groundSpriteID = g_mapSpriteID[curPacked] & 0x1FF;
			t->overlaySpriteID = 0;
		}
	}

	if (!g_debugScenario) {
		Animation_Start(g_table_animation_structure[0], s->o.position, si->layout, s->o.houseID, (uint8)si->iconGroup);
	}

	h = House_Get_ByIndex(s->o.houseID);

	for (i = 0; i < 5; i++) {
		if (h->ai_structureRebuild[i][0] != 0) continue;
		h->ai_structureRebuild[i][0] = s->o.type;
		h->ai_structureRebuild[i][1] = packed;
		break;
	}

	Structure_Free(s);
	Structure_UntargetMe(s);

	h->structuresBuilt = Structure_GetStructuresBuilt(h);
	g_factoryWindowTotal = -1;

	House_UpdateCreditsStorage(s->o.houseID);

	if (g_debugScenario) return;

	switch (s->o.type) {
		case STRUCTURE_WINDTRAP:
			House_CalculatePowerAndCredit(h);
			break;

		case STRUCTURE_OUTPOST:
			House_UpdateRadarState(h);
			break;

		default: break;
	}
}

/**
 * Check if requested structureType can be build on the map with concrete below.
 *
 * @param structureType The type of structure to check for.
 * @param houseID The house to check for.
 * @return True if and only if there are enough slabs available on the map to
 *  build requested structure.
 */
static bool Structure_CheckAvailableConcrete(uint16 structureType, uint8 houseID)
{
	const StructureInfo *si;
	uint16 tileCount;
	uint16 i;

	si = &g_table_structureInfo[structureType];

	tileCount = g_table_structure_layoutTileCount[si->layout];

	if (structureType == STRUCTURE_SLAB_1x1 || structureType == STRUCTURE_SLAB_2x2) return true;

	for (i = 0; i < 4096; i++) {
		bool stop = true;
		uint16 j;

		for (j = 0; j < tileCount; j++) {
			uint16 packed = i + g_table_structure_layoutTiles[si->layout][j];
			/* XXX -- This can overflow, and we should check for that */

			if (Map_GetLandscapeType(packed) == LST_CONCRETE_SLAB && g_map[packed].houseID == houseID) continue;

			stop = false;
			break;
		}

		if (stop) return true;
	}

	return false;
}

/**
 * Cancel the building of object for given structure.
 *
 * @param s The Structure.
 */
void Structure_CancelBuild(Structure *s)
{
	ObjectInfo *oi;

	if (s == NULL || s->o.linkedID == 0xFF) return;

	if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
		Structure *s2 = Structure_Get_ByIndex(s->o.linkedID);
		oi = &g_table_structureInfo[s2->o.type].o;
		Structure_Free(s2);
	} else {
		Unit *u = Unit_Get_ByIndex(s->o.linkedID);
		oi = &g_table_unitInfo[u->o.type].o;
		Unit_Free(u);
	}

	House_Get_ByIndex(s->o.houseID)->credits += ((oi->buildTime - (s->countDown >> 8)) * 256 / oi->buildTime) * oi->buildCredits / 256;

	s->o.flags.s.onHold = false;
	s->countDown = 0;
	s->o.linkedID = 0xFF;
}

/**
 * Make the given Structure build an object.
 *
 * @param s The Structure.
 * @param objectType The type of the object to build or a special value (0xFFFD, 0xFFFE, 0xFFFF).
 *        0xFFFD: set upgrading state.
 *        0xFFFE: pick default object type.
 *        0xFFFF: open factory window.
 * @return ??.
 */
bool Structure_BuildObject(Structure *s, uint16 objectType)
{
	const StructureInfo *si;
	const char *str;
	Object *o;
	ObjectInfo *oi;
	assert(objectType != 0xFFFF); /* Factory window. */

	if (s == NULL) return false;

	si = &g_table_structureInfo[s->o.type];

	if (!si->o.flags.factory) return false;

	Structure_SetRepairingState(s, 0, NULL);

	if (objectType == 0xFFFD) {
		Structure_SetUpgradingState(s, 1, NULL);
		return false;
	}

	if (objectType == 0xFFFF || objectType == 0xFFFE) {
		/* Significantly different from OpenDUNE since we don't
		 * clobber the availability table, and we don't have the
		 * factory window (objectType == 0xFFFF).
		 */
		if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
			for (int i = STRUCTURE_SLAB_1x1; i < STRUCTURE_MAX; i++) {
				if (Structure_GetAvailable(s, i) > 0) {
					s->objectType = i;
					break;
				}
			}

			return false;
		}
		else if (s->o.type != STRUCTURE_STARPORT) {
			for (int i = 0; i < 8; i++) {
				const int u = si->buildableUnits[i];

				if (Structure_GetAvailable(s, u)) {
					s->objectType = u;
					break;
				}
			}

			return false;
		} else {
			s->objectType = objectType;
		}
	}

	if (s->o.type == STRUCTURE_STARPORT) return true;

	if (s->objectType != objectType) Structure_CancelBuild(s);

	if (s->o.linkedID != 0xFF || objectType == 0xFFFF) return false;

	if (s->o.type != STRUCTURE_CONSTRUCTION_YARD) {
		tile32 tile;
		tile.x = 0xFFFF;
		tile.y = 0xFFFF;

		Unit *u = Unit_Create(UNIT_INDEX_INVALID, (uint8)objectType, s->o.houseID, tile, 0);
		if (u != NULL) {
			o = &u->o;
			oi = &g_table_unitInfo[objectType].o;
			str = String_Get_ByIndex(g_table_unitInfo[objectType].o.stringID_full);
		}
		else {
			o = NULL;
		}
	} else {
		Structure *st = Structure_Create(STRUCTURE_INDEX_INVALID, (uint8)objectType, s->o.houseID, 0xFFFF);
		if (st != NULL) {
			o = &st->o;
			oi = &g_table_structureInfo[objectType].o;
			str = String_Get_ByIndex(g_table_structureInfo[objectType].o.stringID_full);

			if (!Structure_CheckAvailableConcrete(objectType, s->o.houseID))
				GUI_DisplayHint(STR_HINT_THERE_ISNT_ENOUGH_OPEN_CONCRETE_TO_PLACE_THIS_STRUCTURE_YOU_MAY_PROCEED_BUT_WITHOUT_ENOUGH_CONCRETE_THE_BUILDING_WILL_NEED_REPAIRS,
						g_table_structureInfo[objectType].o.spriteID);
		}
		else {
			o = NULL;
		}
	}

	s->o.flags.s.onHold = false;

	if (o != NULL) {
		s->o.linkedID = o->index & 0xFF;
		s->objectType = objectType;
		s->countDown = oi->buildTime << 8;

		Structure_SetState(s, STRUCTURE_STATE_BUSY);

		if (s->o.houseID != g_playerHouseID) return true;

		GUI_DisplayText(String_Get_ByIndex(STR_PRODUCTION_OF_S_HAS_STARTED), 2, str);

		return true;
	}

	if (s->o.houseID != g_playerHouseID) return false;

	GUI_DisplayText(String_Get_ByIndex(STR_UNABLE_TO_CREATE_MORE), 2);

	return false;
}

/**
 * Sets or toggle the upgrading state of the given Structure.
 *
 * @param s The Structure.
 * @param value The upgrading state, -1 to toggle.
 * @param w The widget.
 * @return True if and only if the state changed.
 */
bool Structure_SetUpgradingState(Structure *s, int8 state, Widget *w)
{
	bool ret = false;

	if (s == NULL) return false;

	if (state == -1) state = s->o.flags.s.upgrading ? 0 : 1;

	if (state == 0 && s->o.flags.s.upgrading) {
		if (s->o.houseID == g_playerHouseID) {
			GUI_DisplayText(String_Get_ByIndex(STR_UPGRADING_STOPS), 2);
		}

		s->o.flags.s.upgrading = false;
		s->o.flags.s.onHold = false;

		GUI_Widget_MakeNormal(w, false);

		ret = true;
	}

	if (state == 0 || s->o.flags.s.upgrading || s->upgradeTimeLeft == 0) return ret;

	if (s->o.houseID == g_playerHouseID) {
		GUI_DisplayText(String_Get_ByIndex(STR_UPGRADING_STARTS), 2);
	}

	s->o.flags.s.onHold = true;
	s->o.flags.s.repairing = false;
	s->o.flags.s.upgrading = true;

	GUI_Widget_MakeSelected(w, false);

	return true;
}

/**
 * Sets or toggle the repairing state of the given Structure.
 *
 * @param s The Structure.
 * @param value The repairing state, -1 to toggle.
 * @param w The widget.
 * @return True if and only if the state changed.
 */
bool Structure_SetRepairingState(Structure *s, int8 state, Widget *w)
{
	bool ret = false;

	if (s == NULL) return false;

	/* ENHANCEMENT -- If a structure gets damaged during upgrading, pressing the "Upgrading" button silently starts the repair of the structure, and doesn't cancel upgrading. */
	if (g_dune2_enhanced && s->o.flags.s.upgrading) return false;

	if (!s->o.flags.s.allocated) state = 0;

	if (state == -1) state = s->o.flags.s.repairing ? 0 : 1;

	if (state == 0 && s->o.flags.s.repairing) {
		if (s->o.houseID == g_playerHouseID) {
			GUI_DisplayText(String_Get_ByIndex(STR_REPAIRING_STOPS), 2);
		}

		s->o.flags.s.repairing = false;
		s->o.flags.s.onHold = false;

		GUI_Widget_MakeNormal(w, false);

		ret = true;
	}

	if (state == 0 || s->o.flags.s.repairing || s->o.hitpoints == g_table_structureInfo[s->o.type].o.hitpoints) return ret;

	if (s->o.houseID == g_playerHouseID) {
		GUI_DisplayText(String_Get_ByIndex(STR_REPAIRING_STARTS), 2);
	}

	s->o.flags.s.onHold = true;
	s->o.flags.s.repairing = true;

	GUI_Widget_MakeSelected(w, false);

	return true;
}

/**
 * Update the map with the right data for this structure.
 * @param s The structure to update on the map.
 */
void Structure_UpdateMap(Structure *s)
{
	const StructureInfo *si;
	uint16 layoutSize;
	const uint16 *layout;
	uint16 *iconMap;
	int i;

	if (s == NULL) return;
	if (!s->o.flags.s.used) return;
	if (s->o.flags.s.isNotOnMap) return;

	si = &g_table_structureInfo[s->o.type];

	layout = g_table_structure_layoutTiles[si->layout];
	layoutSize = g_table_structure_layoutTileCount[si->layout];

	iconMap = &g_iconMap[g_iconMap[si->iconGroup] + layoutSize + layoutSize];

	for (i = 0; i < layoutSize; i++) {
		uint16 position;
		Tile *t;

		position = Tile_PackTile(s->o.position) + layout[i];

		t = &g_map[position];
		t->houseID = s->o.houseID;
		t->hasStructure = true;
		t->index = s->o.index + 1;

		t->groundSpriteID = iconMap[i] + s->rotationSpriteDiff;

		if (Sprite_IsUnveiled(t->overlaySpriteID)) t->overlaySpriteID = 0;

		Map_Update(position, 0, false);
	}

	if (s->state >= STRUCTURE_STATE_IDLE) {
		uint16 animationIndex = (s->state > STRUCTURE_STATE_READY) ? STRUCTURE_STATE_READY : s->state;

		if (si->animationIndex[animationIndex] == 0xFF) {
			Animation_Start(NULL, s->o.position, si->layout, s->o.houseID, (uint8)si->iconGroup);
		} else {
			uint8 animationID = si->animationIndex[animationIndex];

			assert(animationID < 29);
			Animation_Start(g_table_animation_structure[animationID], s->o.position, si->layout, s->o.houseID, (uint8)si->iconGroup);
		}
	} else {
		Animation_Start(g_table_animation_structure[1], s->o.position, si->layout, s->o.houseID, (uint8)si->iconGroup);
	}
}

static uint32
Structure_GetPrerequisites(const StructureInfo *si, enum HouseType houseID)
{
	const uint8 houseFlag = (1 << houseID);
	uint32 structuresRequired = si->o.structuresRequired;

	/* Desired behaviour: Harkonnen can build WOR without Barracks. */
	/* if (i == STRUCTURE_WOR_TROOPER && s->o.houseID == HOUSE_HARKONNEN && g_campaignID >= 1) {} */

	/* Generalised behaviour: the prerequisite structures are only
	 * required if the owner can build it.
	 */
	for (enum StructureType prereq = STRUCTURE_PALACE; prereq < STRUCTURE_MAX; prereq++) {
		const uint32 prereq_flag = (1 << prereq);

		if ((structuresRequired & prereq_flag) && !(g_table_structureInfo[prereq].o.availableHouse & houseFlag))
			structuresRequired &= ~prereq_flag;
	}

	return structuresRequired;
}

int
Structure_GetAvailable(const Structure *s, int i)
{
	if (s->o.type == STRUCTURE_STARPORT) {
		assert(UNIT_CARRYALL <= i && i <= UNIT_MAX);
		return (g_starportAvailable[i] == 0) ? 0 : 1;
	}

	const uint32 structuresBuilt = House_Get_ByIndex(s->o.houseID)->structuresBuilt;

	if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
		assert(STRUCTURE_SLAB_1x1 <= i && i < STRUCTURE_MAX);

		const StructureInfo *si = &g_table_structureInfo[i];
		uint16 availableCampaign = si->o.availableCampaign[s->o.houseID];
		uint32 structuresRequired = Structure_GetPrerequisites(si, s->o.houseID);

		/* Use per-house tech levels: Harkonnen WOR three levels earlier. */
		/* if (i == STRUCTURE_WOR_TROOPER && s->o.houseID == HOUSE_HARKONNEN && g_campaignID >= 1) {} */

		/* Use per-house tech levels: non-Harkonnen light factory one level earlier. */
		/* if ((s->o.houseID != HOUSE_HARKONNEN) && (i == STRUCTURE_LIGHT_VEHICLE)) {} */

		if (((structuresBuilt & structuresRequired) == structuresRequired) || (s->o.houseID != g_playerHouseID)) {

			if ((g_campaignID >= availableCampaign - 1) && (si->o.availableHouse & (1 << s->o.houseID))) {
				if ((s->upgradeLevel >= si->o.upgradeLevelRequired[s->o.houseID]) || (s->o.houseID != g_playerHouseID)) {
					return 1;
				}
				else if ((s->upgradeTimeLeft != 0) && (s->upgradeLevel + 1 >= si->o.upgradeLevelRequired[s->o.houseID])) {
					return -1;
				}
			}
		}

		/* If the prerequisites were destroyed, allow the current
		 * building to complete but do not allow new ones.
		 */
		if (i == s->objectType && s->o.linkedID != 0xFF)
			return -2;

		return 0;
	}

	else {
		uint16 unitType = i;

		if (unitType > UNIT_MCV)
			return 0;

		/* Note: Raider trike's availableHouse should be Ordos only. */
		/* if (unitType == UNIT_TRIKE && s->creatorHouseID == HOUSE_ORDOS) unitType = UNIT_RAIDER_TRIKE; */

		const UnitInfo *ui = &g_table_unitInfo[unitType];
		uint16 upgradeLevelRequired = ui->o.upgradeLevelRequired[s->creatorHouseID];
		uint16 next_upgrade_level = s->upgradeLevel + 1;

		/* Desired behaviour: Ordos siege tank upgrade shown one upgrade level earlier. */
		/* if (unitType == UNIT_SIEGE_TANK && s->creatorHouseID == HOUSE_ORDOS) {} */

		/* Generalised behaviour: skip upgrade levels that do not unlock units. */
		while (next_upgrade_level < 3) {
			if (Structure_UpgradeUnlocksNewUnit(s, next_upgrade_level))
				break;

			next_upgrade_level++;
		}

		if ((structuresBuilt & ui->o.structuresRequired) != ui->o.structuresRequired) {
			/* If the prerequisites were destroyed, allow the current
			 * building to complete but do not allow new ones.
			 */
			if (unitType == s->objectType && s->o.linkedID != 0xFF)
				return -2;

			return 0;
		}

		if ((ui->o.availableHouse & (1 << s->creatorHouseID)) == 0)
			return 0;

		if (s->upgradeLevel >= upgradeLevelRequired) {
			return 1;
		}
		else if (s->upgradeTimeLeft != 0 && next_upgrade_level >= upgradeLevelRequired) {
			return -1;
		}

		return 0;
	}
}

/**
 * The house is under attack in the form of a structure being hit.
 * @param houseID The house who is being attacked.
 */
void Structure_HouseUnderAttack(uint8 houseID)
{
	PoolFindStruct find;
	House *h;

	h = House_Get_ByIndex(houseID);

	if (houseID != g_playerHouseID && h->flags.doneFullScaleAttack) return;
	h->flags.doneFullScaleAttack = true;

	if (h->flags.human) {
		if (h->timerStructureAttack != 0) return;

		Audio_PlayVoice(VOICE_OUR_BASE_IS_UNDER_ATTACK);

		h->timerStructureAttack = 8;
		return;
	}

	/* ENHANCEMENT -- Dune2 originally only searches for units with type 0 (Carry-all). In result, the rest of this function does nothing. */
	if (!g_dune2_enhanced) return;

	find.houseID = houseID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		const UnitInfo *ui;
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		ui = &g_table_unitInfo[u->o.type];

		if (ui->bulletType == UNIT_INVALID) continue;

		/* XXX -- Dune2 does something odd here. What was their intention? */
		if ((u->actionID == ACTION_GUARD && u->actionID == ACTION_AMBUSH) || u->actionID == ACTION_AREA_GUARD) Unit_SetAction(u, ACTION_HUNT);
	}
}

static int GUI_FactoryWindow_Sorter(const void *a, const void *b)
{
	const FactoryWindowItem *pa = a;
	const FactoryWindowItem *pb = b;

	return pb->sortPriority - pa->sortPriority;
}

void Structure_InitFactoryItems(const Structure *s)
{
	g_factoryWindowTotal = 0;

	memset(g_factoryWindowItems, 0, MAX_FACTORY_WINDOW_ITEMS * sizeof(FactoryWindowItem));

	if (s->o.type == STRUCTURE_STARPORT) {
		Random_Starport_Reseed();
	}

	const StructureInfo *si = &g_table_structureInfo[s->o.type];
	if (s->o.type != STRUCTURE_CONSTRUCTION_YARD) {
		const int end = (s->o.type == STRUCTURE_STARPORT) ? UNIT_MCV + 1 : 8;

		for (int i = 0; i < end; i++) {
			uint16 unitType = (s->o.type == STRUCTURE_STARPORT) ? i : si->buildableUnits[i];

			/* if (unitType == UNIT_TRIKE && s->creatorHouseID == HOUSE_ORDOS && s->o.type != STRUCTURE_STARPORT) unitType = UNIT_RAIDER_TRIKE; */

			if (unitType > UNIT_MCV)
				continue;

			const ObjectInfo *oi = &g_table_unitInfo[unitType].o;
			int available;

			if (s->o.type == STRUCTURE_STARPORT) {
				available = Structure_GetAvailable(s, i);
			}
			else {
				available = Structure_GetAvailable(s, si->buildableUnits[i]);
			}

			if (available == 0)
				continue;

			g_factoryWindowItems[g_factoryWindowTotal].objectType = unitType;
			g_factoryWindowItems[g_factoryWindowTotal].available = available;
			g_factoryWindowItems[g_factoryWindowTotal].sortPriority = oi->sortPriority;
			g_factoryWindowItems[g_factoryWindowTotal].shapeID = oi->spriteID;

			if (s->o.type == STRUCTURE_STARPORT) {
				g_factoryWindowItems[g_factoryWindowTotal].credits = Random_Starport_CalculatePrice(oi->buildCredits);
			}
			else if (available == -1) {
				g_factoryWindowItems[g_factoryWindowTotal].credits = floor(si->o.buildCredits / 40) * 20;
			}
			else {
				g_factoryWindowItems[g_factoryWindowTotal].credits = oi->buildCredits;
			}

			g_factoryWindowTotal++;
		}
	}
	else {
		for (enum StructureType i = 0; i < STRUCTURE_MAX; i++) {
			const ObjectInfo *oi = &g_table_structureInfo[i].o;
			const int available = Structure_GetAvailable(s, i);

			if (available == 0)
				continue;

			g_factoryWindowItems[g_factoryWindowTotal].objectType = i;
			g_factoryWindowItems[g_factoryWindowTotal].available = available;
			g_factoryWindowItems[g_factoryWindowTotal].shapeID = oi->spriteID;

			if (available == -1) {
				g_factoryWindowItems[g_factoryWindowTotal].credits = floor(si->o.buildCredits / 40) * 20;
			}
			else {
				g_factoryWindowItems[g_factoryWindowTotal].credits = oi->buildCredits;
			}

#if 0
			if (i == STRUCTURE_SLAB_1x1 || i == STRUCTURE_SLAB_2x2) {
				g_factoryWindowItems[g_factoryWindowTotal].sortPriority = 0x64;
			}
			else {
				g_factoryWindowItems[g_factoryWindowTotal].sortPriority = oi->sortPriority;
			}
#else
			g_factoryWindowItems[g_factoryWindowTotal].sortPriority = oi->sortPriority;
#endif

			g_factoryWindowTotal++;
		}
	}

	if (g_factoryWindowTotal > 0)
		qsort(g_factoryWindowItems, g_factoryWindowTotal, sizeof(FactoryWindowItem), GUI_FactoryWindow_Sorter);
}

void
Structure_Starport_Restock(enum UnitType type)
{
	if (g_starportAvailable[type] != 0 && g_starportAvailable[type] < 10) {
		if (g_starportAvailable[type] == -1) {
			g_starportAvailable[type] = 1;
		}
		else {
			g_starportAvailable[type]++;
		}
	}
}
