/** @file src/unit.c %Unit routines. */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "enum_string.h"
#include "types.h"
#include "os/common.h"
#include "os/math.h"
#include "os/strings.h"

#include "unit.h"

#include "ai.h"
#include "animation.h"
#include "audio/audio.h"
#include "config.h"
#include "enhancement.h"
#include "explosion.h"
#include "gui/gui.h"
#include "gui/widget.h"
#include "house.h"
#include "map.h"
#include "net/net.h"
#include "net/server.h"
#include "newui/actionpanel.h"
#include "newui/menubar.h"
#include "opendune.h"
#include "pool/pool.h"
#include "pool/pool_house.h"
#include "pool/pool_structure.h"
#include "pool/pool_unit.h"
#include "pool/pool_team.h"
#include "scenario.h"
#include "sprites.h"
#include "string.h"
#include "structure.h"
#include "table/locale.h"
#include "table/sound.h"
#include "team.h"
#include "tile.h"
#include "timer/timer.h"
#include "tools/coord.h"
#include "tools/encoded_index.h"
#include "tools/orientation.h"
#include "tools/random_general.h"


Unit *g_unitActive = NULL;
static Unit *g_unitSelected[MAX_SELECTABLE_UNITS];

/**
 * Number of units of each type available at the starport.
 * \c 0 means not available, \c -1 means \c 0 units, \c >0 means that number of units available.
 */
int16 g_starportAvailable[UNIT_MAX];

Unit *
Unit_FirstSelected(int *iter)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i]) {
			if (iter != NULL)
				*iter = i;

			return g_unitSelected[i];
		}
	}

	if (iter != NULL)
		*iter = 0;

	return NULL;
}

Unit *
Unit_NextSelected(int *iter)
{
	for (int i = *iter + 1; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i]) {
			*iter = i;
			return g_unitSelected[i];
		}
	}

	*iter = 0;
	return NULL;
}

bool
Unit_IsSelected(const Unit *unit)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i] == unit)
			return true;
	}

	return false;
}

bool
Unit_AnySelected(void)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i] != NULL)
			return true;
	}

	return false;
}

void
Unit_AddSelected(Unit *unit)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i] == NULL) {
			g_unitSelected[i] = unit;
			return;
		}
	}
}

void
Unit_Unselect(const Unit *unit)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i] == unit) {
			g_unitSelected[i] = NULL;
			break;
		}
	}

	if ((g_selectionType == SELECTIONTYPE_UNIT) && !Unit_AnySelected()) {
		/* ENHANCEMENT -- When a unit enters or deploys into a
		 * structure, the last tile the Unit was on becomes selected
		 * rather than the entire Structure.
		 */
		if (enhancement_fix_selection_after_entering_structure)
			Map_SetSelection(Tile_PackTile(unit->o.position));

		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
	}
}

void
Unit_SelectType(enum UnitType type)
{
	PoolFindStruct find;
	Unit *u = Unit_FindFirst(&find, g_playerHouseID, type);
	while (u != NULL) {
		int x, y;
		if (Map_IsPositionInViewport(u->o.position, &x, &y)) {		
			Unit_AddSelected(u);
		}
		u = Unit_FindNext(&find);
	}
}

void
Unit_UnselectType(enum UnitType type)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i] == NULL) continue;
		if (g_unitSelected[i]->o.type == type && g_unitSelected[i]->o.houseID == g_playerHouseID) {
			g_unitSelected[i] = NULL;
		}
	}

	if ((g_selectionType == SELECTIONTYPE_UNIT) && !Unit_AnySelected()) {
		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
	}
}

void
Unit_UnselectAllButKeepType(enum UnitType type)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i] == NULL) continue;
		if (g_unitSelected[i]->o.type != type || g_unitSelected[i]->o.houseID != g_playerHouseID) {
			g_unitSelected[i] = NULL;
		}
	}

	if ((g_selectionType == SELECTIONTYPE_UNIT) && !Unit_AnySelected()) {
		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
	}
}

void
Unit_UnselectAll(void)
{
	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		g_unitSelected[i] = NULL;
	}

	if (g_selectionType == SELECTIONTYPE_UNIT)
		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
}

Unit *
Unit_GetForActionPanel(void)
{
	/* Select a representative unit for the action panel. */
	const int priority_table[] = {
		-9, /* UNIT_CARRYALL */
		-9, /* UNIT_ORNITHOPTER */
		15, /* UNIT_INFANTRY */
		20, /* UNIT_TROOPERS */
		 5, /* UNIT_SOLDIER */
		10, /* UNIT_TROOPER */
		-1, /* UNIT_SABOTEUR: sabotage instead of attack */
		60, /* UNIT_LAUNCHER */
		65, /* UNIT_DEVIATOR */
		50, /* UNIT_TANK */
		70, /* UNIT_SIEGE_TANK */
		90, /* UNIT_DEVASTATOR: will destruct! */
		80, /* UNIT_SONIC_TANK */
		30, /* UNIT_TRIKE */
		35, /* UNIT_RAIDER_TRIKE */
		40, /* UNIT_QUAD */
		-2, /* UNIT_HARVESTER: harvest instead of attack */
		-3, /* UNIT_MCV: deploy instead of attack */
	};

	Unit *u;
	Unit *best_unit = NULL;
	int best_priority = -1000;
	int iter;

	for (u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
		int priority = -9;

		if (u->o.type <= UNIT_MCV) {
			priority = priority_table[u->o.type];

			if (Unit_GetHouseID(u) == g_playerHouseID)
				priority += 100;
		}

		if (best_priority < priority) {
			best_priority = priority;
			best_unit = u;
		}
	}

	return best_unit;
}

enum UnitActionType
Unit_GetSimilarAction(const uint16 *actions, enum UnitActionType actionID)
{
	enum UnitActionType secondary_action = ACTION_INVALID;

	for (int i = 0; i < 4; i++) {
		if (actionID == actions[i])
			return actionID;

		if (secondary_action != ACTION_INVALID)
			continue;

		if ((actionID == ACTION_ATTACK  && actions[i] == ACTION_HARVEST)
		 || (actionID == ACTION_HARVEST && actions[i] == ACTION_ATTACK)
		 || (actionID == ACTION_RETREAT && actions[i] == ACTION_RETURN)
		 || (actionID == ACTION_RETURN  && actions[i] == ACTION_RETREAT)
		 || (actionID == ACTION_GUARD   && actions[i] == ACTION_STOP)
		 || (actionID == ACTION_STOP    && actions[i] == ACTION_GUARD)) {
			secondary_action = actionID;
		}
	}

	return secondary_action;
}

/**
 * Rotate a unit (or his top).
 *
 * @param unit The Unit to operate on.
 * @param level 0 = base, 1 = top (turret etc).
 */
static void Unit_Rotate(Unit *unit, uint16 level)
{
	int8 target;
	int8 current;
	int8 newCurrent;
	int16 diff;

	assert(level == 0 || level == 1);

	if (unit->orientation[level].speed == 0) return;

	target = unit->orientation[level].target;
	current = unit->orientation[level].current;
	diff = target - current;

	if (diff > 128) diff -= 256;
	if (diff < -128) diff += 256;
	diff = abs(diff);

	newCurrent = current + unit->orientation[level].speed;

	if (abs(unit->orientation[level].speed) >= diff) {
		unit->orientation[level].speed = 0;
		newCurrent = target;
	}

	unit->orientation[level].current = newCurrent;

	if (Orientation_256To16(newCurrent) == Orientation_256To16(current) && Orientation_256To8(newCurrent) == Orientation_256To8(current)) return;

	Unit_UpdateMap(2, unit);
}

tile32
Unit_GetNextDestination(const Unit *u)
{
	if ((enhancement_true_game_speed_adjustment && u->speedPerTick == 255) ||
		(u->speedRemainder + Tools_AdjustToGameSpeed(u->speedPerTick, 1, 255, false) > 0xFF)) {
		const int dist = min(u->speed, Tile_GetDistance(u->o.position, u->currentDestination) + 16);

		return Tile_MoveByDirectionUnbounded(u->o.position, u->orientation[0].current, dist);
	} else {
		return u->o.position;
	}
}

static void Unit_MovementTick(Unit *unit)
{
	uint16 speed;

	unit->lastPosition = Unit_GetNextDestination(unit);

	if (unit->speed == 0) return;

	/* ENHANCEMENT -- always move units with maximum speedPerTick. */
	if (enhancement_true_game_speed_adjustment && unit->speedPerTick == 255) {
		speed = unit->speedRemainder + 256;
	} else {
		speed = unit->speedRemainder + Tools_AdjustToGameSpeed(unit->speedPerTick, 1, 255, false);
	}

	if (speed > 0xFF) {
		Unit_Move(unit, min(unit->speed, Tile_GetDistance(unit->o.position, unit->currentDestination) + 16));
	}

	unit->speedRemainder = speed & 0xFF;
}

/**
 * Loop over all units, performing various of tasks.
 */
void GameLoop_Unit(void)
{
	PoolFindStruct find;
	bool tickMovement  = false;
	bool tickRotation  = false;
	bool tickBlinking  = false;
	bool tickMoveIndicator  = false;
	bool tickUnknown4  = false;
	bool tickScript    = false;
	bool tickUnknown5  = false;
	bool tickDeviation = false;

	if (g_debugScenario) return;

	if (g_tickUnitMovement <= g_timerGame) {
		tickMovement = true;
		g_tickUnitMovement = g_timerGame + 3;
	}

	if (g_tickUnitRotation <= g_timerGame) {
		tickRotation = true;
		g_tickUnitRotation = g_timerGame + Tools_AdjustToGameSpeed(4, 2, 8, true);
	}

	if (g_tickUnitBlinking <= g_timerGame) {
		tickBlinking = true;
		g_tickUnitBlinking = g_timerGame + Tools_AdjustToGameSpeed(6, 3, 12, true);
	}

	if (g_tickUnitMoveIndicator <= g_timerGame) {
		tickMoveIndicator = true;
		g_tickUnitMoveIndicator = g_timerGame + 5;
	}

	if (g_tickUnitUnknown4 <= g_timerGame) {
		tickUnknown4 = true;
		g_tickUnitUnknown4 = g_timerGame + 20;
	}

	if (g_tickUnitScript <= g_timerGame) {
		tickScript = true;
		g_tickUnitScript = g_timerGame + 5;
	}

	if (g_tickUnitUnknown5 <= g_timerGame) {
		tickUnknown5 = true;
		g_tickUnitUnknown5 = g_timerGame + 5;
	}

	if (g_tickUnitDeviation <= g_timerGame) {
		tickDeviation = true;
		g_tickUnitDeviation = g_timerGame + 60;
	}

	for (Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {
		const UnitInfo *ui = &g_table_unitInfo[u->o.type];

		g_scriptCurrentObject    = &u->o;
		g_scriptCurrentStructure = NULL;
		g_scriptCurrentUnit      = u;
		g_scriptCurrentTeam      = NULL;

		if (u->o.flags.s.isNotOnMap) continue;

		/* Do not unveil new tiles for air units (ornithopters), but
		 * allow them to refresh previously scouted tiles for vision.
		 */
		if (enhancement_fog_of_war)
			Unit_RefreshFog(UNVEILCAUSE_UNIT_VISION, u, ui->flags.isGroundUnit);

		if (tickUnknown4 && u->targetAttack != 0 && ui->o.flags.hasTurret) {
			tile32 tile;

			tile = Tools_Index_GetTile(u->targetAttack);

			Unit_SetOrientation(u, Tile_GetDirection(u->o.position, tile), false, 1);
		}

		if (tickMovement) {
			Unit_MovementTick(u);

			if (u->fireDelay != 0) {
				if (ui->movementType == MOVEMENT_WINGER && !ui->flags.isNormalUnit) {
					tile32 tile;

					tile = u->currentDestination;

					if (Tools_Index_GetType(u->targetAttack) == IT_UNIT && g_table_unitInfo[Tools_Index_GetUnit(u->targetAttack)->o.type].movementType == MOVEMENT_WINGER) {
						tile = Tools_Index_GetTile(u->targetAttack);
					}

					Unit_SetOrientation(u, Tile_GetDirection(u->o.position, tile), false, 0);
				}

				u->fireDelay--;
			}
		}

		if (tickRotation) {
			Unit_Rotate(u, 0);
			if (ui->o.flags.hasTurret) Unit_Rotate(u, 1);
		}

		if (tickBlinking && u->blinkCounter != 0) {
			u->blinkCounter--;
			if ((u->blinkCounter % 2) != 0) {
				u->o.flags.s.isHighlighted = true;
			} else {
				u->o.flags.s.isHighlighted = false;
			}

			Unit_UpdateMap(2, u);
		}

		if (tickMoveIndicator && u->moveIndicatorCounter != 0) {
			u->moveIndicatorCounter--;
			if (u->moveIndicatorCounter > 0 && u->moveIndicatorCounter < 10) {
				u->showMoveIndicator = true;
			}
			else {
				u->showMoveIndicator = false;
			}
		}

		if (tickDeviation) Unit_Deviation_Decrease(u, 1);

		if (ui->movementType != MOVEMENT_WINGER && Object_GetByPackedTile(Tile_PackTile(u->o.position)) == NULL) Unit_UpdateMap(1, u);

		if (tickUnknown5) {
			if (u->timer == 0) {
				if ((ui->movementType == MOVEMENT_FOOT && u->speed != 0) || u->o.flags.s.isSmoking) {
					if (u->spriteOffset >= 0) {
						u->spriteOffset &= 0x3F;
						u->spriteOffset++;

						Unit_UpdateMap(2, u);

						u->timer = ui->animationSpeed / 5;
						if (u->o.flags.s.isSmoking) {
							u->timer = 3;
							if (u->spriteOffset > 32) {
								u->o.flags.s.isSmoking = false;
								u->spriteOffset = 0;
							}
						}
					}
				}

				if (u->o.type == UNIT_ORNITHOPTER && u->o.flags.s.allocated && u->spriteOffset >= 0) {
					u->spriteOffset &= 0x3F;
					u->spriteOffset++;

					Unit_UpdateMap(2, u);

					u->timer = 1;
				}

				if (u->o.type == UNIT_HARVESTER) {
					if (u->actionID == ACTION_HARVEST || u->o.flags.s.isSmoking) {
						u->spriteOffset &= 0x3F;
						u->spriteOffset++;

						Unit_UpdateMap(2, u);

						u->timer = 4;
					} else {
						if (u->spriteOffset != 0) {
							Unit_UpdateMap(2, u);

							u->spriteOffset = 0;
						}
					}
				}
			} else {
				u->timer--;
			}
		}

		if (tickScript) {
			if (u->o.script.delay == 0) {
				if (Script_IsLoaded(&u->o.script)) {
					/* The scripts compare
					 *
					 *  u->o.script.variables[3] == g_playerHouseID to
					 *  Script_Unit_GetInfo(14) == Unit_GetHouseID(u)
					 *
					 * to determine if a unit is human controllable.
					 */
					const enum HouseType houseID = Unit_GetHouseID(u);
					u->o.script.variables[3]
						= House_IsHuman(houseID) ? houseID : HOUSE_INVALID;

					for (int opcodesLeft = SCRIPT_UNIT_OPCODES_PER_TICK + 2;
							opcodesLeft > 0 && u->o.script.delay == 0;
							opcodesLeft--) {
						if (!Script_Run(&u->o.script))
							break;
					}
				}
			} else {
				u->o.script.delay--;
			}
		}

		if (u->nextActionID == ACTION_INVALID) continue;
		if (Unit_IsMoving(u)) continue;

		Unit_Server_SetAction(u, u->nextActionID);
		u->nextActionID = ACTION_INVALID;
	}
}

/**
 * Get the HouseID of a unit. This is not always u->o.houseID, as a unit can be
 *  deviated by the Ordos.
 *
 * @param u Unit to get the HouseID of.
 * @return The HouseID of the unit, which might be deviated.
 */
uint8 Unit_GetHouseID(const Unit *u)
{
	if (u->deviated != 0) {
		/* ENHANCEMENT -- Deviated units always belong to Ordos, no matter who did the deviating. */
		if (enhancement_nonordos_deviation) return u->deviatedHouse;
		return HOUSE_ORDOS;
	}
	return u->o.houseID;
}

/**
 * Convert the name of a unit to the type value of that unit, or
 *  UNIT_INVALID if not found.
 */
uint8 Unit_StringToType(const char *name)
{
	uint8 type;
	if (name == NULL) return UNIT_INVALID;

	for (type = 0; type < UNIT_MAX; type++) {
		if (strcasecmp(g_table_unitInfo[type].o.name, name) == 0) return type;
	}

	/* ENHANCEMENT -- CHOAM and reinforcement typos: "Devistator", "Thopter" instead of Devastator, Ornithopter. */
	if (enhancement_fix_scenario_typos) {
		if (strcasecmp(name, "Thopter") == 0)
			return UNIT_ORNITHOPTER;

		if (strcasecmp(name, "Devistator") == 0)
			return UNIT_DEVASTATOR;
	}

	return UNIT_INVALID;
}

/**
 * Convert the name of an action to the type value of that action, or
 *  ACTION_INVALID if not found.
 */
uint8 Unit_ActionStringToType(const char *name)
{
	uint8 type;
	if (name == NULL) return ACTION_INVALID;

	for (type = 0; type < ACTION_MAX; type++) {
		if (strcasecmp(g_table_actionInfo[type].name, name) == 0) return type;
	}

	return ACTION_INVALID;
}

/**
 * Convert the name of a movement to the type value of that movement, or
 *  MOVEMENT_INVALID if not found.
 */
uint8 Unit_MovementStringToType(const char *name)
{
	uint8 type;
	if (name == NULL) return MOVEMENT_INVALID;

	for (type = 0; type < MOVEMENT_MAX; type++) {
		/* ENHANCEMENT -- AI team typos: "Track", "Wheel" instead of Tracked, Wheeled. */
		if (enhancement_fix_scenario_typos) {
			if (strncasecmp(g_table_movementTypeName[type], name, 5) == 0) return type;
		} else {
			if (strcasecmp(g_table_movementTypeName[type], name) == 0) return type;
		}
	}

	return MOVEMENT_INVALID;
}

/**
 * Create a new Unit.
 *
 * @param index The new index of the Unit, or UNIT_INDEX_INVALID to assign one.
 * @param typeID The type of the new Unit.
 * @param houseID The House of the new Unit.
 * @param position To where on the map this Unit should be transported, or TILE_INVALID for not on the map yet.
 * @param orientation Orientation of the Unit.
 * @return The new created Unit, or NULL if something failed.
 */
Unit *Unit_Create(uint16 index, uint8 typeID, uint8 houseID, tile32 position, int8 orientation)
{
	const UnitInfo *ui;
	Unit *u;

	if (houseID >= HOUSE_MAX) return NULL;
	if (typeID >= UNIT_MAX) return NULL;

	ui = &g_table_unitInfo[typeID];
	u = Unit_Allocate(index, typeID, houseID);
	if (u == NULL) return NULL;

	u->o.houseID = houseID;

	Unit_SetOrientation(u, orientation, true, 0);
	Unit_SetOrientation(u, orientation, true, 1);

	Unit_SetSpeed(u, 0);
	u->speedRemainder = 0;

	u->lastPosition     = position;
	u->o.position       = position;
	u->o.hitpoints      = ui->o.hitpoints;
	u->currentDestination.x = 0;
	u->currentDestination.y = 0;
	u->originEncoded    = 0x0000;
	u->route[0]         = 0xFF;

	if (position.x != 0xFFFF || position.y != 0xFFFF) {
		u->originEncoded = Unit_FindClosestRefinery(u);
		u->targetLast    = position;
		u->targetPreLast = position;
	}

	u->o.linkedID    = 0xFF;
	u->o.script.delay = 0;
	u->actionID      = ACTION_GUARD;
	u->nextActionID  = ACTION_INVALID;
	u->fireDelay     = 0;
	u->distanceToDestination = 0x7FFF;
	u->targetMove    = 0x0000;
	u->amount        = 0;
	u->wobbleIndex   = 0;
	u->spriteOffset  = 0;
	u->blinkCounter  = 0;
	u->showMoveIndicator = 0;
	u->moveIndicatorCounter = 0;
	u->timer   = 0;

	Script_Reset(&u->o.script, g_scriptUnit);

	u->o.flags.s.allocated = true;

	if (ui->movementType == MOVEMENT_TRACKED) {
		if (Tools_Random_256() < g_table_houseInfo[houseID].degradingChance) {
			u->o.flags.s.degrades = true;
		}
	}

	if (ui->movementType == MOVEMENT_WINGER) {
		Unit_SetSpeed(u, 255);
	} else {
		if ((position.x != 0xFFFF || position.y != 0xFFFF) && Unit_IsTileOccupied(u)) {
			Unit_Free(u);
			return NULL;
		}
	}

	if ((position.x == 0xFFFF) && (position.y == 0xFFFF)) {
		u->o.flags.s.isNotOnMap = true;
		return u;
	}

	Unit_UpdateMap(1, u);

	Unit_Server_SetAction(u,
			House_IsHuman(houseID) ? ui->o.actionsPlayer[3] : ui->actionAI);

	return u;
}

/**
 * Sets the action the given unit will execute.
 *
 * @param u The Unit to set the action for.
 * @param action The action.
 */
void
Unit_Server_SetAction(Unit *u, enum UnitActionType action)
{
	const ActionInfo *ai;

	if (u == NULL) return;
	if (u->actionID == ACTION_DESTRUCT || u->actionID == ACTION_DIE || action == ACTION_INVALID) return;

	/* ENHANCEMENT -- When sandworms are insatiable, change ambush to
	 * area guard to prevent eating too many units in quick succession.
	 */
	if (enhancement_insatiable_sandworms && u->o.type == UNIT_SANDWORM && action == ACTION_AMBUSH)
		action = ACTION_AREA_GUARD;

	ai = &g_table_actionInfo[action];

	switch (ai->switchType) {
		case 0:
			if (Unit_IsMoving(u)) {
				u->nextActionID = action;
				return;
			}
			/* FALL-THROUGH */
		case 1:
			u->actionID = action;
			u->nextActionID = ACTION_INVALID;
			u->currentDestination.x = 0;
			u->currentDestination.y = 0;
			u->o.script.delay = 0;
			Script_Reset(&u->o.script, g_scriptUnit);
			u->o.script.variables[0] = action;
			Script_Load(&u->o.script, u->o.type);
			return;

		case 2:
			u->o.script.variables[0] = action;
			Script_LoadAsSubroutine(&u->o.script, u->o.type);
			return;

		default: return;
	}
}

/**
 * Adds the specified unit to the specified team.
 *
 * @param u The unit to add to the team.
 * @param t The team to add the unit to.
 * @return Amount of space left in the team.
 */
uint16 Unit_AddToTeam(Unit *u, Team *t)
{
	if (t == NULL || u == NULL) return 0;

	u->team = t->index + 1;
	t->members++;

	return t->maxMembers - t->members;
}

/**
 * Removes the specified unit from its team.
 *
 * @param u The unit to remove from the team it is in.
 * @return Amount of space left in the team.
 */
uint16 Unit_RemoveFromTeam(Unit *u)
{
	Team *t;

	if (u == NULL) return 0;
	if (u->team == 0) return 0;

	t = Team_Get_ByIndex(u->team - 1);

	t->members--;
	u->team = 0;

	return t->maxMembers - t->members;
}

/**
 * Gets the team of the given unit.
 *
 * @param u The unit to get the team of.
 * @return The team.
 */
Team *Unit_GetTeam(Unit *u)
{
	if (u == NULL) return NULL;
	if (u->team == 0) return NULL;
	return Team_Get_ByIndex(u->team - 1);
}

/**
 * ?? Sorts unit array and count enemy/allied units.
 */
void Unit_Sort(void)
{
	PoolFindStruct find;
	House *h;

	h = g_playerHouse;
	h->unitCountEnemy = 0;
	h->unitCountAllied = 0;

	for (uint16 i = 0; i < g_unitFindCount - 1; i++) {
		Unit *u1;
		Unit *u2;
		uint16 y1;
		uint16 y2;

		u1 = g_unitFindArray[i];
		u2 = g_unitFindArray[i + 1];
		y1 = u1->o.position.y;
		y2 = u2->o.position.y;
		if (g_table_unitInfo[u1->o.type].movementType == MOVEMENT_FOOT) y1 -= 0x100;
		if (g_table_unitInfo[u2->o.type].movementType == MOVEMENT_FOOT) y2 -= 0x100;

		if ((int16)y1 > (int16)y2) {
			g_unitFindArray[i] = u2;
			g_unitFindArray[i + 1] = u1;
		}
	}

	for (const Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {
		if ((u->o.seenByHouses & (1 << g_playerHouseID)) != 0 && !u->o.flags.s.isNotOnMap) {
			if (House_AreAllied(u->o.houseID, g_playerHouseID)) {
				h->unitCountAllied++;
			} else {
				h->unitCountEnemy++;
			}
		}
	}
}

/**
 * Get the unit on the given packed tile.
 *
 * @param packed The packed tile to get the unit from.
 * @return The unit.
 */
Unit *Unit_Get_ByPackedTile(uint16 packed)
{
	Tile *tile;

	if (Tile_IsOutOfMap(packed)) return NULL;

	tile = &g_map[packed];
	if (!tile->hasUnit) return NULL;
	return Unit_Get_ByIndex(tile->index - 1);
}

/**
 * Determines whether a move order into the given structure is OK for
 * a particular unit.
 *
 * It handles orders to invade enemy buildings as well as going into
 * a friendly structure (e.g. refinery, repair facility).
 *
 * @param unit The Unit to operate on.
 * @param s The Structure to operate on.
 * @return
 * 0 - invalid movement
 * 1 - valid movement, will try to get close to the structure
 * 2 - valid movement, will attempt to damage/conquer the structure
 */
uint16 Unit_IsValidMovementIntoStructure(Unit *unit, Structure *s)
{
	const StructureInfo *si;
	const UnitInfo *ui;
	uint16 unitEnc;
	uint16 structEnc;

	if (unit == NULL || s == NULL) return 0;

	si = &g_table_structureInfo[s->o.type];
	ui = &g_table_unitInfo[unit->o.type];

	unitEnc = Tools_Index_Encode(unit->o.index, IT_UNIT);
	structEnc = Tools_Index_Encode(s->o.index, IT_STRUCTURE);

	/* Movement into structure of other owner. */
	if (Unit_GetHouseID(unit) != s->o.houseID) {
		/* Saboteur can always enter houses */
		if (unit->o.type == UNIT_SABOTEUR && unit->targetMove == structEnc) return 2;
		/* Entering houses is only possible for foot-units and if the structure is conquerable.
		 * Everyone else can only move close to the building. */
		if (ui->movementType == MOVEMENT_FOOT && si->o.flags.conquerable) {
			/* ENHANCEMENT -- due to the low weapon range, soldiers tend to capture structures when ordered to attack. */
			if (enhancement_fix_firing_logic && (unit->actionID == ACTION_ATTACK))
				return 1;

			return unit->targetMove == structEnc ? 2 : 1;
		}
		return 0;
	}

	/* Prevent movement if target structure does not accept the unit type. */
	if ((si->enterFilter & (1 << unit->o.type)) == 0) return 0;

	/* TODO -- Not sure. */
	if (s->o.script.variables[4] == unitEnc) return 2;

	/* Enter only if structure not linked to any other unit already. */
	return s->o.linkedID == 0xFF ? 1 : 0;
}

/**
 * Sets the destination for the given unit.
 *
 * @param u The unit to set the destination for.
 * @param destination The destination (encoded index).
 */
void Unit_SetDestination(Unit *u, uint16 destination)
{
	Structure *s;

	if (u == NULL) return;

	if (AI_IsBrutalAI(u->o.houseID)) {
		/* For brutal AI, consider joining (or forming) a squad and
		 * using the squad's destination instead.
		 */
		destination = UnitAI_GetSquadDestination(u, destination);
	}

	if (!Tools_Index_IsValid(destination)) return;
	if (u->targetMove == destination) return;

	if (Tools_Index_GetType(destination) == IT_TILE) {
		Unit *u2;
		uint16 packed;

		packed = Tools_Index_Decode(destination);

		u2 = Unit_Get_ByPackedTile(packed);
		if (u2 != NULL) {
			if (u != u2) destination = Tools_Index_Encode(u2->o.index, IT_UNIT);
		} else {
			s = Structure_Get_ByPackedTile(packed);
			if (s != NULL) destination = Tools_Index_Encode(s->o.index, IT_STRUCTURE);
		}
	}

	s = Tools_Index_GetStructure(destination);
	if (s != NULL && s->o.houseID == Unit_GetHouseID(u)) {
		if (Unit_IsValidMovementIntoStructure(u, s) == 1 || g_table_unitInfo[u->o.type].movementType == MOVEMENT_WINGER) {
			Object_Script_Variable4_Link(Tools_Index_Encode(u->o.index, IT_UNIT), destination);
		}
	}

	u->targetMove = destination;
	u->route[0]   = 0xFF;
}

/**
 * Get the priority a target unit has for a given unit. The higher the value,
 *  the more serious it should look at the target.
 *
 * @param unit The unit looking at a target.
 * @param target The unit to look at.
 * @return The priority of the target.
 */
uint16 Unit_GetTargetUnitPriority(Unit *unit, Unit *target)
{
	const UnitInfo *targetInfo;
	const UnitInfo *unitInfo;
	uint16 distance;
	uint16 priority;

	if (unit == NULL || target == NULL) return 0;
	if (unit == target) return 0;

	if (!target->o.flags.s.allocated) return 0;
	if ((target->o.seenByHouses & (1 << Unit_GetHouseID(unit))) == 0) return 0;

	if (House_AreAllied(Unit_GetHouseID(unit), Unit_GetHouseID(target))) return 0;

	unitInfo   = &g_table_unitInfo[unit->o.type];
	targetInfo = &g_table_unitInfo[target->o.type];

	if (!targetInfo->o.flags.priority) return 0;

	if (targetInfo->movementType == MOVEMENT_WINGER) {
		if (!unitInfo->o.flags.targetAir) return 0;

		/* SINGLE PLAYER -- Do not fire at the player's flying units
		 * that are currently in unscouted territory.  Presumably,
		 * this is so that the player won't lose flying units without
		 * knowing about it.
		 */
		if (g_host_type == HOSTTYPE_NONE) {
			if (target->o.houseID == g_playerHouseID
					&& !Map_IsPositionUnveiled(g_playerHouseID, Tile_PackTile(target->o.position))) {
				return 0;
			}
		}
	}

	if (!Map_IsValidPosition(Tile_PackTile(target->o.position))) return 0;

	distance = Tile_GetDistanceRoundedUp(unit->o.position, target->o.position);

	if (!Map_IsValidPosition(Tile_PackTile(unit->o.position))) {
		if (targetInfo->fireDistance >= distance) return 0;
	}

	priority = targetInfo->o.priorityTarget + targetInfo->o.priorityBuild;
	if (distance != 0) priority = (priority / distance) + 1;

	if (AI_IsBrutalAI(Unit_GetHouseID(unit))) {
		/* Make brutal AI prefer to attack its own deviated vehicles with light guns. */
		bool was_ally = House_AreAllied(Unit_GetHouseID(unit), target->o.houseID);

		if (was_ally) {
			if (g_table_unitInfo[unit->o.type].damage >= target->o.hitpoints) {
				return 0;
			} else if (g_table_unitInfo[unit->o.type].damage <= 10) {
				priority += 100;
			} else {
				priority = 1;
			}
		}
	}

	if (priority > 0x7D00) return 0x7D00;
	return priority;
}

/**
 * Finds the closest refinery a harvester can go to.
 *
 * @param unit The unit to find the closest refinery for.
 * @return 1 if unit->originEncoded was not 0, else 0.
 */
uint16 Unit_FindClosestRefinery(Unit *unit)
{
	uint16 res;
	const Structure *s = NULL;
	uint16 mind = 0;
	uint16 d;
	PoolFindStruct find;

	res = (unit->originEncoded == 0) ? 0 : 1;

	if (unit->o.type != UNIT_HARVESTER) {
		unit->originEncoded = Tools_Index_Encode(Tile_PackTile(unit->o.position), IT_TILE);
		return res;
	}

	/* Find non-busy refinery. */
	for (const Structure *s2 = Structure_FindFirst(&find, Unit_GetHouseID(unit), STRUCTURE_REFINERY);
			s2 != NULL;
			s2 = Structure_FindNext(&find)) {
		if (s2->state != STRUCTURE_STATE_BUSY) continue;
		d = Tile_GetDistance(unit->o.position, s2->o.position);
		if (mind != 0 && d >= mind) continue;
		mind = d;
		s = s2;
	}

	/* Find busy refinery. */
	if (s == NULL) {
		for (const Structure *s2 = Structure_FindFirst(&find, Unit_GetHouseID(unit), STRUCTURE_REFINERY);
				s2 != NULL;
				s2 = Structure_FindNext(&find)) {
			d = Tile_GetDistance(unit->o.position, s2->o.position);
			if (mind != 0 && d >= mind) continue;
			mind = d;
			s = s2;
		}
	}

	if (s != NULL) unit->originEncoded = Tools_Index_Encode(s->o.index, IT_STRUCTURE);

	return res;
}

/**
 * Sets the position of the given unit.
 *
 * @param u The Unit to set the position for.
 * @position The position.
 * @return True if and only if the position changed.
 */
bool Unit_SetPosition(Unit *u, tile32 position)
{
	const UnitInfo *ui;

	if (u == NULL) return false;

	ui = &g_table_unitInfo[u->o.type];
	u->o.flags.s.isNotOnMap = false;

	u->o.position = Tile_Center(position);

	if (u->originEncoded == 0) Unit_FindClosestRefinery(u);

	u->o.script.variables[4] = 0;

	if (Unit_IsTileOccupied(u)) {
		u->o.flags.s.isNotOnMap = true;
		return false;
	}

	u->currentDestination.x = 0;
	u->currentDestination.y = 0;
	u->targetMove = 0;
	u->targetAttack = 0;

	if (Map_IsUnveiledToHouse(g_playerHouseID, Tile_PackTile(u->o.position))) {
		/* A new unit being delivered fresh from the factory; force a seenByHouses
		 *  update and add it to the statistics etc. */
		u->o.seenByHouses &= ~(1 << u->o.houseID);
		Unit_HouseUnitCount_Add(u, g_playerHouseID);
	}

	if (!House_IsHuman(u->o.houseID)
			|| u->o.type == UNIT_HARVESTER
			|| u->o.type == UNIT_SABOTEUR) {
		Unit_Server_SetAction(u, ui->actionAI);
	} else {
		Unit_Server_SetAction(u, ui->o.actionsPlayer[3]);
	}

	u->spriteOffset = 0;

	Unit_UpdateMap(1, u);

	return true;
}

/**
 * Remove the Unit from the game, doing all required administration for it, like
 *  deselecting it, remove it from the radar count, stopping scripts, ..
 *
 * @param u The Unit to remove.
 */
void Unit_Remove(Unit *u)
{
	if (u == NULL) return;

	u->o.flags.s.allocated = true;
	Unit_UntargetMe(u);

	Unit_Unselect(u);

	u->o.flags.s.bulletIsBig = true;
	Unit_UpdateMap(0, u);

	Unit_HouseUnitCount_Remove(u);

	Script_Reset(&u->o.script, g_scriptUnit);

	Unit_Free(u);
}

/**
 * Gets the best target unit for the given unit.
 *
 * @param u The Unit to get the best target for.
 * @param mode How to determine the best target.
 * @return The best target or NULL if none found.
 */
Unit *Unit_FindBestTargetUnit(Unit *u, uint16 mode)
{
	tile32 position;
	uint16 distance;
	PoolFindStruct find;
	Unit *best = NULL;
	uint16 bestPriority = 0;

	if (u == NULL) return NULL;

	position = u->o.position;
	if (u->originEncoded == 0) {
		u->originEncoded = Tools_Index_Encode(Tile_PackTile(position), IT_TILE);
	} else {
		position = Tools_Index_GetTile(u->originEncoded);
	}

	distance = g_table_unitInfo[u->o.type].fireDistance << 8;
	if (mode == 2) distance <<= 1;

	for (Unit *target = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			target != NULL;
			target = Unit_FindNext(&find)) {
		if (mode != 0 && mode != 4) {
			if (mode == 1) {
				if (Tile_GetDistance(u->o.position, target->o.position) > distance) continue;
			}
			if (mode == 2) {
				if (Tile_GetDistance(position, target->o.position) > distance) continue;
			}
		}

		const uint16 priority = Unit_GetTargetUnitPriority(u, target);
		if ((int16)priority > (int16)bestPriority) {
			best = target;
			bestPriority = priority;
		}
	}

	if (bestPriority == 0) return NULL;

	return best;
}

/**
 * Get the priority for a target. Various of things have influence on this score,
 *  most noticeable the movementType of the target, his distance to you, and
 *  if he is moving/firing.
 * @note It only considers units on sand.
 *
 * @param unit The Unit that is requesting the score.
 * @param target The Unit that is being targeted.
 * @return The priority of the target.
 */
static uint16 Unit_Sandworm_GetTargetPriority(Unit *unit, Unit *target)
{
	uint16 res;
	uint16 distance;

	if (unit == NULL || target == NULL) return 0;

	const uint16 packed = Tile_PackTile(target->o.position);
	if (!g_table_landscapeInfo[Map_GetLandscapeType(packed)].isSand)
		return 0;

	/* SINGLE PLAYER -- Sandworms will only target units in scouted
	 * territory.  Presumably this was done to prevent sandworms
	 * attacking stationary CPU units, and out-of-sight worm attacks.
	 */
	if (g_host_type == HOSTTYPE_NONE) {
		if (!Map_IsPositionUnveiled(g_playerHouseID, packed))
			return 0;
	}

	switch(g_table_unitInfo[target->o.type].movementType) {
		case MOVEMENT_FOOT:      res = 0x64;   break;
		case MOVEMENT_TRACKED:   res = 0x3E8;  break;
		case MOVEMENT_HARVESTER: res = 0x3E8;  break;
		case MOVEMENT_WHEELED:   res = 0x1388; break;
		default:                 res = 0;      break;
	}

	if (target->speed != 0 || target->fireDelay != 0) res *= 4;

	distance = Tile_GetDistanceRoundedUp(unit->o.position, target->o.position);

	if (distance != 0 && res != 0) res /= distance;
	if (distance < 2) res *= 2;

	return res;
}

/**
 * Find the best target, based on the score. Only considers units on sand.
 *
 * @param unit The unit to search a target for.
 * @return A target Unit, or NULL if none is found.
 */
Unit *Unit_Sandworm_FindBestTarget(Unit *unit)
{
	Unit *best = NULL;
	PoolFindStruct find;
	uint16 bestPriority = 0;

	if (unit == NULL) return NULL;

	for (Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {
		const uint16 priority = Unit_Sandworm_GetTargetPriority(unit, u);

		if (priority >= bestPriority) {
			best = u;
			bestPriority = priority;
		}
	}

	if (bestPriority == 0) return NULL;

	return best;
}

/**
 * Initiate the first movement of a Unit when the pathfinder has found a route.
 *
 * @param unit The Unit to operate on.
 * @return True if movement was initiated (not blocked etc).
 */
bool Unit_StartMovement(Unit *unit)
{
	const UnitInfo *ui;
	int8 orientation;
	uint16 packed;
	uint16 type;
	tile32 position;
	uint16 speed;
	int16 score;

	if (unit == NULL) return false;

	ui = &g_table_unitInfo[unit->o.type];

	orientation = (int8)((unit->orientation[0].current + 16) & 0xE0);

	Unit_SetOrientation(unit, orientation, true, 0);
	Unit_SetOrientation(unit, orientation, false, 1);

	position = Tile_MoveByOrientation(unit->o.position, orientation);

	packed = Tile_PackTile(position);

	unit->distanceToDestination = 0x7FFF;

	score = Unit_GetTileEnterScore(unit, packed, orientation / 32);

	if (score > 255 || score == -1) return false;

	type = Map_GetLandscapeType(packed);
	if (type == LST_STRUCTURE) type = LST_CONCRETE_SLAB;

	speed = g_table_landscapeInfo[type].movementSpeed[ui->movementType];

	if (unit->o.type == UNIT_SABOTEUR && type == LST_WALL) speed = 255;
	unit->o.flags.s.isSmoking = false;

	/* ENHANCEMENT -- the flag is never set to false in original Dune2; in result, once the wobbling starts, it never stops. */
	if (enhancement_fix_everlasting_unit_wobble) {
		unit->o.flags.s.isWobbling = g_table_landscapeInfo[type].letUnitWobble;
	} else {
		if (g_table_landscapeInfo[type].letUnitWobble) unit->o.flags.s.isWobbling = true;
	}

	if ((ui->o.hitpoints / 2) > unit->o.hitpoints && ui->movementType != MOVEMENT_WINGER) speed -= speed / 4;

	Unit_SetSpeed(unit, speed);

	if (ui->movementType != MOVEMENT_SLITHER) {
		tile32 positionOld;

		positionOld = unit->o.position;
		unit->o.position = position;

		Unit_UpdateMap(1, unit);

		unit->o.position = positionOld;
	}

	unit->currentDestination = position;

	Unit_Deviation_Decrease(unit, 10);

	return true;
}

/**
 * Set the target for the given unit.
 *
 * @param unit The Unit to set the target for.
 * @param encoded The encoded index of the target.
 */
void Unit_SetTarget(Unit *unit, uint16 encoded)
{
	if (unit == NULL || !Tools_Index_IsValid(encoded)) return;
	if (unit->targetAttack == encoded) return;

	if (Tools_Index_GetType(encoded) == IT_TILE) {
		uint16 packed;
		Unit *u;

		packed = Tools_Index_Decode(encoded);

		u = Unit_Get_ByPackedTile(packed);
		if (u != NULL) {
			encoded = Tools_Index_Encode(u->o.index, IT_UNIT);
		} else {
			Structure *s;

			s = Structure_Get_ByPackedTile(packed);
			if (s != NULL) {
				encoded = Tools_Index_Encode(s->o.index, IT_STRUCTURE);
			}
		}
	}

	if (Tools_Index_Encode(unit->o.index, IT_UNIT) == encoded) {
		encoded = Tools_Index_Encode(Tile_PackTile(unit->o.position), IT_TILE);
	}

	unit->targetAttack = encoded;

	if (!g_table_unitInfo[unit->o.type].o.flags.hasTurret) {
		unit->targetMove = encoded;
		unit->route[0] = 0xFF;
	}
}

/**
 * Decrease deviation counter for the given unit.
 *
 * @param unit The Unit to decrease counter for.
 * @param amount The amount to decrease.
 * @return True if and only if the unit lost deviation.
 */
bool Unit_Deviation_Decrease(Unit *unit, uint16 amount)
{
	const UnitInfo *ui;

	if (unit == NULL || unit->deviated == 0) return false;

	ui = &g_table_unitInfo[unit->o.type];

	if (!ui->flags.isNormalUnit) return false;

	if (amount == 0) {
		amount = g_table_houseInfo[unit->o.houseID].toughness;
	}

	if (unit->deviated > amount) {
		unit->deviated -= amount;
		return false;
	}

	unit->deviated = 0;

	unit->o.flags.s.bulletIsBig = true;
	Unit_UpdateMap(2, unit);
	unit->o.flags.s.bulletIsBig = false;

	if (House_IsHuman(unit->o.houseID)) {
		Unit_Server_SetAction(unit, ui->o.actionsPlayer[3]);
	} else {
		Unit_Server_SetAction(unit, ui->actionAI);
	}

	Unit_UntargetMe(unit);
	unit->targetAttack = 0;
	unit->targetMove = 0;

	return true;
}

/**
 * Remove fog arount the given unit.
 *
 * @param unit The Unit to remove fog around.
 */
void
Unit_RefreshFog(enum TileUnveilCause cause, const Unit *unit, bool unveil)
{
	uint16 fogUncoverRadius;

	if (unit == NULL) return;
	if (unit->o.flags.s.isNotOnMap) return;
	if (unit->o.flags.s.inTransport) return;
	if ((unit->o.position.x == 0xFFFF && unit->o.position.y == 0xFFFF) || (unit->o.position.x == 0 && unit->o.position.y == 0)) return;

	fogUncoverRadius = Unit_GetFogUncoverRadius(unit->o.type, g_table_unitInfo[unit->o.type].o.fogUncoverRadius);
	if (fogUncoverRadius == 0) return;

	Tile_RefreshFogInRadius(House_GetAllies(Unit_GetHouseID(unit)), cause,
			unit->o.position, fogUncoverRadius, unveil);
}

void
Unit_RemoveFog(enum TileUnveilCause cause, const Unit *unit)
{
	Unit_RefreshFog(cause, unit, true);
}

/**
 * Deviate the given unit.
 *
 * @param unit The Unit to deviate.
 * @param probability The probability for deviation to succeed.
 * @param houseID House controlling the deviator.
 * @return True if and only if the unit beacame deviated.
 */
bool Unit_Deviate(Unit *unit, uint16 probability, uint8 houseID)
{
	const UnitInfo *ui;

	if (unit == NULL) return false;

	ui = &g_table_unitInfo[unit->o.type];

	if (!ui->flags.isNormalUnit) return false;
	if (unit->deviated != 0) return false;
	if (ui->flags.isNotDeviatable) return false;

	if (probability == 0) probability = g_table_houseInfo[unit->o.houseID].toughness;

	if (!House_IsHuman(unit->o.houseID)) {
		probability -= probability / 8;
	}

	if (Tools_Random_256() >= probability) return false;

	if (!enhancement_nonordos_deviation)
		houseID = HOUSE_ORDOS;

	unit->deviated = 120;
	unit->deviatedHouse = houseID;

	Unit_UpdateMap(2, unit);

	if (House_IsHuman(houseID)) {
		/* Make brutal AI know about its own deviated units. */
		if (AI_IsBrutalAI(unit->o.houseID)) {
			unit->o.seenByHouses = 0xFF;
		}

		Unit_Server_SetAction(unit, ui->o.actionsPlayer[3]);
	} else {
		/* Make brutal AI destruct devastators if outcome is desirable. */
		if (AI_IsBrutalAI(houseID)
				&& UnitAI_ShouldDestructDevastator(unit)) {
			Unit_Server_SetAction(unit, ACTION_DESTRUCT);
		} else {
			Unit_Server_SetAction(unit, ui->actionAI);
		}
	}

	Unit_UntargetMe(unit);
	unit->targetAttack = 0;
	unit->targetMove = 0;

	return true;
}

/**
 * Moves the given unit.
 *
 * @param unit The Unit to move.
 * @param distance The maximum distance to pass through.
 * @return ??.
 */
bool Unit_Move(Unit *unit, uint16 distance)
{
	const UnitInfo *ui;
	uint16 d;
	uint16 packed;
	tile32 newPosition;
	bool ret;
	tile32 currentDestination;
	bool isSpiceBloom = false;
	bool isSpecialBloom = false;

	if (unit == NULL || !unit->o.flags.s.used) return false;

	ui = &g_table_unitInfo[unit->o.type];

	newPosition = Tile_MoveByDirection(unit->o.position, unit->orientation[0].current, distance);

	if ((newPosition.x == unit->o.position.x) && (newPosition.y == unit->o.position.y)) return false;

	if (!Tile_IsValid(newPosition)) {
		if (!ui->flags.mustStayInMap) {
			Unit_Remove(unit);
			return true;
		}

		if (unit->o.flags.s.byScenario && unit->o.linkedID == 0xFF && unit->o.script.variables[4] == 0) {
			Unit_Remove(unit);
			return true;
		}

		const int r = (Tools_Random_256() & 0xF);

		newPosition = unit->o.position;
		Unit_SetOrientation(unit, unit->orientation[0].current + r, false, 0);

		/* Due to a carryall's turning radius, it will sometimes not
		 * rotate even though the current orientation is not the
		 * target orientation.  This can cause carryalls on the edge
		 * of the map to get stuck.
		 */
		if ((unit->orientation[0].current != unit->orientation[0].target) &&
		    (unit->orientation[0].speed == 0)) {
			Unit_SetOrientation(unit, unit->orientation[0].current + r, true, 0);
		}
	}

	if (ui->flags.canWobble && unit->o.flags.s.isWobbling) {
		unit->wobbleIndex = Tools_Random_256() & 7;
	} else if (ui->o.flags.blurTile) {
		/* Use wobbleIndex for blur effect instead of a global
		 * variable because we only want the unit's wobble amount to
		 * change when it moves.
		 *
		 * Sandworms are in three segments, so increase its frames by
		 * 3 at a time.
		 */
		if (unit->o.type == UNIT_SANDWORM) {
			unit->wobbleIndex = (unit->wobbleIndex + 3) & 7;
		} else {
			unit->wobbleIndex = (unit->wobbleIndex + 1) & 7;
		}
	} else {
		unit->wobbleIndex = 0;
	}

	d = Tile_GetDistance(newPosition, unit->currentDestination);
	packed = Tile_PackTile(newPosition);

	if (ui->flags.isTracked && d < 48) {
		Unit *u;
		u = Unit_Get_ByPackedTile(packed);

		/* Driving over a foot unit */
		if (u != NULL && g_table_unitInfo[u->o.type].movementType == MOVEMENT_FOOT && u->o.flags.s.allocated) {
			Unit_Unselect(u);
			Unit_UntargetMe(u);
			u->o.script.variables[1] = 1;
			Unit_Server_SetAction(u, ACTION_DIE);
		} else {
			uint16 type = Map_GetLandscapeType(packed);
			/* Produce tracks in the sand */
			if ((type == LST_NORMAL_SAND || type == LST_ENTIRELY_DUNE) && g_map[packed].overlaySpriteID == 0) {
				uint8 animationID = Orientation_256To8(unit->orientation[0].current);

				assert(animationID < 8);
				Animation_Start(g_table_animation_unitMove[animationID], unit->o.position, 0, unit->o.houseID, 5);
			}
		}
	}

	Unit_UpdateMap(0, unit);

	if (ui->movementType == MOVEMENT_WINGER) {
		unit->o.flags.s.animationFlip = !unit->o.flags.s.animationFlip;
	}

	currentDestination = unit->currentDestination;
	distance = Tile_GetDistance(newPosition, currentDestination);

	if (unit->o.type == UNIT_SONIC_BLAST) {
		Unit *u;
		uint16 damage;

		damage = (unit->o.hitpoints / 4) + 1;
		ret = false;

		u = Unit_Get_ByPackedTile(packed);

		if (u != NULL) {
			if (!g_table_unitInfo[u->o.type].flags.sonicProtection) {
				Unit_Damage(u, damage, 0);
			}
		} else {
			Structure *s;

			s = Structure_Get_ByPackedTile(packed);

			if (s != NULL) {
				/* ENHANCEMENT -- make sonic blast trigger counter attack, but
				 * do not warn about base under attack (original behaviour). */
				if (g_dune2_enhanced
						&& !House_IsHuman(s->o.houseID)
						&& !House_AreAllied(s->o.houseID, unit->o.houseID)) {
					Structure_HouseUnderAttack(s->o.houseID);
				}

				Structure_Damage(s, damage, 0);
			} else {
				if (Map_GetLandscapeType(packed) == LST_WALL && g_table_structureInfo[STRUCTURE_WALL].o.hitpoints > damage) Tools_Random_256();
			}
		}

		if (unit->o.hitpoints < (ui->damage / 2)) {
			unit->o.flags.s.bulletIsBig = true;
		}

		if (--unit->o.hitpoints == 0 || unit->fireDelay == 0) {
			Unit_Remove(unit);
		}
	} else {
		if (unit->o.type == UNIT_BULLET) {
			uint16 type = Map_GetLandscapeType(Tile_PackTile(newPosition));
			if (type == LST_WALL || type == LST_STRUCTURE) {
				if (Tools_Index_GetType(unit->originEncoded) == IT_STRUCTURE) {
					if (g_map[Tile_PackTile(newPosition)].houseID == unit->o.houseID) {
						type = LST_NORMAL_SAND;
					}
				}
			}

			if (type == LST_WALL || type == LST_STRUCTURE || type == LST_ENTIRELY_MOUNTAIN) {
				unit->o.position = newPosition;

				Map_MakeExplosion((ui->explosionType + unit->o.hitpoints / 10) & 3, unit->o.position, unit->o.hitpoints, unit->originEncoded);

				Unit_Remove(unit);
				return true;
			}
		}

		ret = (unit->distanceToDestination < distance || distance < 16) ? true : false;

		if (ret) {
			if (ui->flags.isBullet) {
				if (unit->fireDelay == 0 || unit->o.type == UNIT_MISSILE_TURRET) {
					if (unit->o.type == UNIT_MISSILE_HOUSE) {
						uint8 i;

						for (i = 0; i < 17; i++) {
							static const int16 offsetX[17] = { 0, 0, 200, 256, 200, 0, -200, -256, -200, 0, 400, 512, 400, 0, -400, -512, -400 };
							static const int16 offsetY[17] = { 0, -256, -200, 0, 200, 256, 200, 0, -200, -512, -400, 0, 400, 512, 400, 0, -400 };
							tile32 p = newPosition;
							p.y += offsetY[i];
							p.x += offsetX[i];

							if (Tile_IsValid(p)) {
								Map_MakeExplosion(ui->explosionType, p, 200, 0);
							}
						}
					} else if (ui->explosionType != 0xFFFF) {
						if (ui->flags.impactOnSand && g_map[Tile_PackTile(unit->o.position)].index == 0 && Map_GetLandscapeType(Tile_PackTile(unit->o.position)) == LST_NORMAL_SAND) {
							Map_MakeExplosion(EXPLOSION_SAND_BURST, newPosition, unit->o.hitpoints, unit->originEncoded);
						} else if (unit->o.type == UNIT_MISSILE_DEVIATOR) {
							Map_DeviateArea(ui->explosionType, newPosition, 32, unit->o.houseID);
						} else {
							Map_MakeExplosion((ui->explosionType + unit->o.hitpoints / 20) & 3, newPosition, unit->o.hitpoints, unit->originEncoded);
						}
					}

					Unit_Remove(unit);
					return true;
				}
			} else if (ui->flags.isGroundUnit) {
				if (Unit_IsMoving(unit)) newPosition = currentDestination;
				unit->targetPreLast = unit->targetLast;
				unit->targetLast    = unit->o.position;
				unit->currentDestination.x = 0;
				unit->currentDestination.y = 0;

				if (unit->o.flags.s.degrades && (Tools_Random_256() & 3) == 0) {
					Unit_Damage(unit, 1, 0);
				}

				if (unit->o.type == UNIT_SABOTEUR) {
					bool detonate = (Map_GetLandscapeType(Tile_PackTile(newPosition)) == LST_WALL);

					if (!detonate) {
						if (enhancement_targetted_sabotage) {
							/* ENHANCEMENT -- Only detonate if the action was a sabotage or a targetted sabotage. */
							detonate = ((unit->actionID == ACTION_SABOTAGE) || (unit->actionID == ACTION_MOVE && unit->detonateAtTarget)) &&
								(Tile_GetDistance(newPosition, Tools_Index_GetTile(unit->targetMove)) < 16);
						} else if (enhancement_fix_firing_logic) {
							/* ENHANCEMENT -- Saboteurs tend to forget their goal, depending on terrain and game speed: to blow up on reaching their destination. */
							detonate = (unit->targetMove != 0 && Tile_GetDistance(newPosition, Tools_Index_GetTile(unit->targetMove)) < 16);
						} else {
							detonate = (unit->targetMove != 0 && Tile_GetDistance(unit->o.position, Tools_Index_GetTile(unit->targetMove)) < 32);
						}
					}

					if (detonate) {
						Map_MakeExplosion(EXPLOSION_SABOTEUR_DEATH, newPosition, 500, 0);

						/* ENHANCEMENT -- Use Unit_Remove so that the saboteur is cleared from the map. */
						Unit_Remove(unit);
						return true;
					}
				}

				Unit_SetSpeed(unit, 0);

				if (unit->targetMove == Tools_Index_Encode(packed, IT_TILE)) {
					unit->targetMove = 0;
				}

				{
					Structure *s;

					s = Structure_Get_ByPackedTile(packed);
					if (s != NULL) {
						unit->targetPreLast.x = 0;
						unit->targetPreLast.y = 0;
						unit->targetLast.x    = 0;
						unit->targetLast.y    = 0;
						Unit_EnterStructure(unit, s);
						return true;
					}
				}

				if (unit->o.type != UNIT_SANDWORM) {
					if (g_map[packed].groundSpriteID == g_bloomSpriteID) {
						isSpiceBloom = true;
					} else if (g_map[packed].groundSpriteID == g_bloomSpriteID + 1) {
						isSpecialBloom = true;
					}
				}

				/* For brutal AI following a plan, check if our ambush has been detected. */
				if (unit->aiSquad != SQUADID_INVALID) {
					uint16 enemy = UnitAI_GetAnyEnemyInRange(unit);

					if (enemy != 0)
						UnitAI_AbortMission(unit, enemy);
				}
			}
		}
	}

	unit->distanceToDestination = distance;
	unit->o.position = newPosition;

	Unit_UpdateMap(1, unit);

	if (isSpecialBloom)
		Map_Bloom_ExplodeSpecial(packed, Unit_GetHouseID(unit));

	if (isSpiceBloom)
		Map_Bloom_ExplodeSpice(packed, 1 << Unit_GetHouseID(unit));

	return ret;
}

/**
 * Applies damages to the given unit.
 *
 * @param unit The Unit to apply damages on.
 * @param damage The amount of damage to apply.
 * @param range ??.
 * @return True if and only if the unit has no hitpoints left.
 */
bool Unit_Damage(Unit *unit, uint16 damage, uint16 range)
{
	const UnitInfo *ui;
	bool alive = false;
	uint8 houseID;

	if (unit == NULL || !unit->o.flags.s.allocated) return false;

	ui = &g_table_unitInfo[unit->o.type];

	if (!ui->flags.isNormalUnit && unit->o.type != UNIT_SANDWORM) return false;

	if (unit->o.hitpoints != 0) alive = true;

	if (unit->o.hitpoints >= damage) {
		unit->o.hitpoints -= damage;
	} else {
		unit->o.hitpoints = 0;
	}

	Unit_Deviation_Decrease(unit, 0);

	houseID = Unit_GetHouseID(unit);

	if (unit->o.hitpoints == 0) {
		const uint16 packed = Tile_PackTile(unit->o.position);

		Unit_RemovePlayer(unit);

		if (unit->o.type == UNIT_HARVESTER)
			Map_FillCircleWithSpice(packed, unit->amount / 32);

		if (unit->o.type == UNIT_SABOTEUR) {
			Server_Send_PlayVoiceAtTile(FLAG_HOUSE_ALL,
					VOICE_SABOTEUR_DESTROYED, packed);
		} else if (!ui->o.flags.noMessageOnDeath && alive) {
			if (g_campaignID > 3) {
				Server_Send_PlayVoiceAtTile(FLAG_HOUSE_ALL,
						VOICE_HARKONNEN_UNIT_DESTROYED + houseID, packed);
			} else {
				Server_Send_PlayVoiceAtTile(1 << houseID,
						VOICE_HARKONNEN_UNIT_DESTROYED + houseID, packed);
				Server_Send_PlayVoiceAtTile(FLAG_HOUSE_ALL & ~(1 << houseID),
						VOICE_ENEMY_UNIT_DESTROYED, packed);
			}
		}

		Unit_Server_SetAction(unit, ACTION_DIE);
		return true;
	}

	if (range != 0) {
		Map_MakeExplosion((damage < 25) ? EXPLOSION_IMPACT_SMALL : EXPLOSION_IMPACT_MEDIUM, unit->o.position, 0, 0);
	}

	if (!House_IsHuman(houseID)
			&& unit->actionID == ACTION_AMBUSH
			&& unit->o.type != UNIT_HARVESTER) {
		Unit_Server_SetAction(unit, ACTION_ATTACK);
	}

	/* For brutal AI, give up sneak attack if we are attacked. */
	UnitAI_AbortMission(unit, 0);

	if (unit->o.hitpoints >= ui->o.hitpoints / 2) return false;

	if (unit->o.type == UNIT_SANDWORM) {
		Unit_Server_SetAction(unit, ACTION_DIE);
	}

	if (unit->o.type == UNIT_TROOPERS || unit->o.type == UNIT_INFANTRY) {
		unit->o.type += 2;
		ui = &g_table_unitInfo[unit->o.type];
		unit->o.hitpoints = ui->o.hitpoints;

		Unit_UpdateMap(2, unit);

		if (Tools_Random_256() < g_table_houseInfo[unit->o.houseID].toughness) {
			Unit_Server_SetAction(unit, ACTION_RETREAT);
		}

		if (enhancement_infantry_squad_death_animations) {
			uint16 animationUnitID = g_table_landscapeInfo[Map_GetLandscapeType(Tile_PackTile(unit->o.position))].isSand ? 0 : 1;
			Animation_Start(g_table_animation_unitScript2[animationUnitID], unit->o.position, 0, Unit_GetHouseID(unit), 4);
		}
	}

	if (ui->movementType != MOVEMENT_TRACKED && ui->movementType != MOVEMENT_HARVESTER && ui->movementType != MOVEMENT_WHEELED) return false;

	unit->o.flags.s.isSmoking = true;
	unit->spriteOffset = 0;
	unit->timer = 0;

	return false;
}

/**
 * Untarget the given Unit.
 *
 * @param unit The Unit to untarget.
 */
void
Unit_UntargetEncodedIndex(uint16 encoded)
{
	PoolFindStruct find;

	for (Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
			u != NULL;
			u = Unit_FindNext(&find)) {
		if (u->targetMove == encoded) u->targetMove = 0;
		if (u->targetAttack == encoded) u->targetAttack = 0;
		if (u->o.script.variables[4] == encoded) Object_Script_Variable4_Clear(&u->o);
	}
}

void Unit_UntargetMe(Unit *unit)
{
	PoolFindStruct find;
	uint16 encoded = Tools_Index_Encode(unit->o.index, IT_UNIT);

	Object_Script_Variable4_Clear(&unit->o);

	Unit_UntargetEncodedIndex(encoded);

	for (Structure *s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
			s != NULL;
			s = Structure_FindNext(&find)) {
		if (s->o.type != STRUCTURE_TURRET && s->o.type != STRUCTURE_ROCKET_TURRET) continue;
		if (s->o.script.variables[2] == encoded) s->o.script.variables[2] = 0;
	}

	UnitAI_DetachFromSquad(unit);
	Unit_RemoveFromTeam(unit);

	for (Team *t = Team_FindFirst(&find, HOUSE_INVALID);
			t != NULL;
			t = Team_FindNext(&find)) {
		if (t->target == encoded) t->target = 0;
	}
}

/**
 * Set the new orientation of the unit.
 *
 * @param unit The Unit to operate on.
 * @param orientation The new orientation of the unit.
 * @param rotateInstantly If true, rotation is instant. Else the unit turns over the next few ticks slowly.
 * @param level 0 = base, 1 = top (turret etc).
 */
void Unit_SetOrientation(Unit *unit, int8 orientation, bool rotateInstantly, uint16 level)
{
	int16 diff;

	assert(level == 0 || level == 1);

	if (unit == NULL) return;

	unit->orientation[level].speed = 0;
	unit->orientation[level].target = orientation;

	if (rotateInstantly) {
		unit->orientation[level].current = orientation;
		return;
	}

	if (unit->orientation[level].current == orientation) return;

	/* ENHANCEMENT -- Carryalls sometimes circle around a tile when
	 * delivering reinforcements due to the turning radius.  Make them
	 * go straight ahead a little further before turning around.
	 */
	if (g_dune2_enhanced && (unit->o.type == UNIT_CARRYALL) && (level == 0)) {
		/* 10281 = integral of _stepX[0] .. _stepX[127]
		 *
		 * is the translation for turning pi radians, or twice turning radius,
		 * when moving a distance of 128 per angle.  Add a small fudge factor.
		 */
		const int turning_radius = 32 + 10281 * unit->speed / (2 * 128 * (g_table_unitInfo[unit->o.type].turningSpeed * 4));
		tile32 circle;

		circle = Tile_MoveByDirectionUnbounded(unit->o.position, unit->orientation[0].current + 64, turning_radius);
		if (Tile_GetDistance(circle, unit->currentDestination) < turning_radius)
			return;

		circle = Tile_MoveByDirectionUnbounded(unit->o.position, unit->orientation[0].current - 64, turning_radius);
		if (Tile_GetDistance(circle, unit->currentDestination) < turning_radius)
			return;
	}

	unit->orientation[level].speed = g_table_unitInfo[unit->o.type].turningSpeed * 4;

	diff = orientation - unit->orientation[level].current;

	if ((diff > -128 && diff < 0) || diff > 128) {
		unit->orientation[level].speed = -unit->orientation[level].speed;
	}
}

/**
 * Selects the given unit.
 *
 * @param unit The Unit to select.
 */
void Unit_Select(Unit *unit)
{
	int iter;
	assert(unit != NULL);

	if (Unit_IsSelected(unit))
		return;

	if (unit != NULL && !unit->o.flags.s.allocated && !g_debugGame) {
		unit = NULL;
	}

	if (unit != NULL && (unit->o.seenByHouses & (1 << g_playerHouseID)) == 0 && !g_debugGame) {
		unit = NULL;
	}

	if (Unit_AnySelected()) {
		for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
			Unit_UpdateMap(2, u);
		}
	}

	if (unit == NULL) {
		Unit_UnselectAll();

		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
		return;
	}

	if (Unit_GetHouseID(unit) == g_playerHouseID) {
		const UnitInfo *ui;

		ui = &g_table_unitInfo[unit->o.type];

		/* Plays the 'reporting' sound file. */
		if (unit->o.type != UNIT_SANDWORM) {
			const enum SampleID sampleID
				= (ui->movementType == MOVEMENT_FOOT)
				? SAMPLE_YES_SIR : SAMPLE_REPORTING;

			Audio_PlaySample(sampleID, 255, 0.0);
		}

		GUI_DisplayHint(g_playerHouseID, ui->o.hintStringID, ui->o.spriteID);
	}

	for (int i = 0; i < MAX_SELECTABLE_UNITS; i++) {
		if (g_unitSelected[i] == NULL) {
			g_unitSelected[i] = unit;
			GUI_ChangeSelectionType(SELECTIONTYPE_UNIT);
			break;
		}
	}

	for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
		Unit_UpdateMap(2, u);
	}

	Map_SetSelectionObjectPosition(0xFFFF);
}

/**
 * Create a unit (and a carryall if needed).
 *
 * @param houseID The House of the new Unit.
 * @param typeID The type of the new Unit.
 * @param destination To where on the map this Unit should move.
 * @return The new created Unit, or NULL if something failed.
 */
Unit *
Unit_CreateWrapper(uint8 houseID, enum UnitType typeID, uint16 destination)
{
	House *h;
	int8 orientation;
	Unit *unit;
	Unit *carryall;

	const uint16 packed = Map_Server_FindLocationTile(Tools_Random_256() & 3, houseID);
	tile32 tile = Tile_UnpackTile(packed);

	h = House_Get_ByIndex(houseID);

	{
		tile32 t;
		t.x = 0x2000;
		t.y = 0x2000;
		orientation = Tile_GetDirection(tile, t);
	}

	if (g_table_unitInfo[typeID].movementType == MOVEMENT_WINGER) {
		g_validateStrictIfZero++;
		unit = Unit_Create(UNIT_INDEX_INVALID, typeID, houseID, tile, orientation);
		g_validateStrictIfZero--;

		if (unit == NULL) return NULL;

		unit->o.flags.s.byScenario = true;

		if (destination != 0) {
			Unit_SetDestination(unit, destination);
		}

		return unit;
	}

	g_validateStrictIfZero++;
	carryall = Unit_Create(UNIT_INDEX_INVALID, UNIT_CARRYALL, houseID, tile, orientation);
	g_validateStrictIfZero--;

	if (carryall == NULL) {
		if (typeID == UNIT_HARVESTER && h->harvestersIncoming == 0) h->harvestersIncoming++;
		return NULL;
	}

	carryall->o.flags.s.byScenario = true;

	tile.x = 0xFFFF;
	tile.y = 0xFFFF;

	g_validateStrictIfZero++;
	unit = Unit_Create(UNIT_INDEX_INVALID, typeID, houseID, tile, 0);
	g_validateStrictIfZero--;

	if (unit == NULL) {
		Unit_Remove(carryall);
		if (typeID == UNIT_HARVESTER && h->harvestersIncoming == 0) h->harvestersIncoming++;
		return NULL;
	}

	carryall->o.flags.s.inTransport = true;
	carryall->o.linkedID = unit->o.index & 0xFF;
	if (typeID == UNIT_HARVESTER) unit->amount = 1;

	if (destination != 0) {
		Unit_SetDestination(carryall, destination);
	}

	return unit;
}

/**
 * Find a target around the given packed tile.
 *
 * @param packed The packed tile around where to look.
 * @return A packed tile where a Unit/Structure is, or the given packed tile if nothing found.
 */
uint16 Unit_FindTargetAround(uint16 packed)
{
	static const int16 around[] = {0, -1, 1, -64, 64, -65, -63, 65, 63};

	uint8 i;

	if (enhancement_i_mean_where_i_clicked)
		return packed;

	if (g_selectionType == SELECTIONTYPE_PLACE) return packed;

	if (Structure_Get_ByPackedTile(packed) != NULL) return packed;

	if (Map_GetLandscapeType(packed) == LST_BLOOM_FIELD) return packed;

	for (i = 0; i < lengthof(around); i++) {
		Unit *u;

		u = Unit_Get_ByPackedTile(packed + around[i]);
		if (u == NULL) continue;

		return Tile_PackTile(u->o.position);
	}

	return packed;
}

/**
 * Check if the position the unit is on is already occupied.
 *
 * @param unit The Unit to operate on.
 * @return True if and only if the position of the unit is already occupied.
 */
bool Unit_IsTileOccupied(Unit *unit)
{
	const UnitInfo *ui;
	uint16 packed;
	Unit *unit2;
	uint16 speed;

	if (unit == NULL) return true;

	ui = &g_table_unitInfo[unit->o.type];
	packed = Tile_PackTile(unit->o.position);

	speed = g_table_landscapeInfo[Map_GetLandscapeType(packed)].movementSpeed[ui->movementType];
	if (speed == 0) return true;

	if (unit->o.type == UNIT_SANDWORM || ui->movementType == MOVEMENT_WINGER) return false;

	unit2 = Unit_Get_ByPackedTile(packed);
	if (unit2 != NULL && unit2 != unit) {
		if (House_AreAllied(Unit_GetHouseID(unit2), Unit_GetHouseID(unit))) return true;
		if (ui->movementType != MOVEMENT_TRACKED) return true;
		if (g_table_unitInfo[unit2->o.type].movementType != MOVEMENT_FOOT) return true;
	}

	return (Structure_Get_ByPackedTile(packed) != NULL);
}

/**
 * Set the speed of a Unit.
 *
 * @param unit The Unit to operate on.
 * @param speed The new speed of the unit (a percent value between 0 and 255).
 */
void Unit_SetSpeed(Unit *unit, uint16 speed)
{
	uint16 speedPerTick;

	assert(unit != NULL);

	speedPerTick = 0;

	unit->speed          = 0;
	unit->speedRemainder = 0;
	unit->speedPerTick   = 0;

	if (speed == 0) return;

	if (unit->o.type == UNIT_HARVESTER) {
		speed = ((255 - unit->amount) * speed) / 256;
	}

	if (speed == 0 || speed >= 256) {
		unit->movingSpeed = 0;
		return;
	}

	unit->movingSpeed = speed & 0xFF;
	speed = g_table_unitInfo[unit->o.type].movingSpeedFactor * speed / 256;
	speed = Tools_AdjustToGameSpeed(speed, 1, 255, false);

	speedPerTick = speed << 4;

	if ((speed >> 4) != 0) {
		speedPerTick = 255;

		if (!enhancement_true_unit_movement_speed) {
			speed >>= 4;
			speed <<= 4;
		}
	} else {
		speed = 16;
	}

	unit->speed = speed & 0xFF;
	unit->speedPerTick = speedPerTick & 0xFF;
}

/**
 * Create a new bullet Unit.
 *
 * @param position Where on the map this bullet Unit is created.
 * @param typeID The type of the new bullet Unit.
 * @param houseID The House of the new bullet Unit.
 * @param damage The hitpoints of the new bullet Unit.
 * @param target The target of the new bullet Unit.
 * @return The new created Unit, or NULL if something failed.
 */
Unit *
Unit_CreateBullet(tile32 position, enum UnitType type, uint8 houseID, uint16 damage, uint16 target)
{
	const UnitInfo *ui;
	tile32 tile;

	if (!Tools_Index_IsValid(target)) return NULL;

	ui = &g_table_unitInfo[type];
	tile = Tools_Index_GetTile(target);

	const enum HouseFlag houses
		= (g_campaign_selected == CAMPAIGNID_MULTIPLAYER)
		? Map_FindHousesInRadius(position, 9)
		: FLAG_HOUSE_ALL;

	switch (type) {
		case UNIT_MISSILE_HOUSE:
		case UNIT_MISSILE_ROCKET:
		case UNIT_MISSILE_TURRET:
		case UNIT_MISSILE_DEVIATOR:
		case UNIT_MISSILE_TROOPER: {
			int8 orientation;
			Unit *bullet;
			Unit *u;

			orientation = Tile_GetDirection(position, tile);

			bullet = Unit_Create(UNIT_INDEX_INVALID, type, houseID, position, orientation);
			if (bullet == NULL) return NULL;

			Server_Send_PlaySoundAtTile(houses,
					ui->bulletSound, position);

			bullet->targetAttack = target;
			bullet->o.hitpoints = damage;
			bullet->currentDestination = tile;

			if (ui->flags.notAccurate) {
				bullet->currentDestination = Tile_MoveByRandom(tile, (Tools_Random_256() & 0xF) != 0 ? Tile_GetDistance(position, tile) / 256 + 8 : Tools_Random_256() + 8, false);
			}

			bullet->fireDelay = ui->fireDistance & 0xFF;

			u = Tools_Index_GetUnit(target);
			if (u != NULL && g_table_unitInfo[u->o.type].movementType == MOVEMENT_WINGER) {
				bullet->fireDelay <<= 1;
			}

			if (type != UNIT_MISSILE_HOUSE) {
				Tile_RemoveFogInRadius(houses, UNVEILCAUSE_BULLET_FIRED,
						bullet->o.position, 2);
			}

			return bullet;
		}

		case UNIT_BULLET:
		case UNIT_SONIC_BLAST: {
			int8 orientation;
			tile32 t;
			Unit *bullet;

			orientation = Tile_GetDirection(position, tile);

			t = Tile_MoveByDirection(Tile_MoveByDirection(position, 0, 32), orientation, 128);

			bullet = Unit_Create(UNIT_INDEX_INVALID, type, houseID, t, orientation);
			if (bullet == NULL) return NULL;

			if (type == UNIT_SONIC_BLAST) {
				bullet->fireDelay = ui->fireDistance & 0xFF;
			}

			bullet->currentDestination = tile;
			bullet->o.hitpoints = damage;

			if (damage > 15) bullet->o.flags.s.bulletIsBig = true;

			Tile_RemoveFogInRadius(houses, UNVEILCAUSE_BULLET_FIRED,
					bullet->o.position, 2);

			return bullet;
		}

		default: return NULL;
	}
}

/**
 * Display status text for the given unit.
 *
 * @param unit The Unit to display status text for.
 */
void Unit_DisplayStatusText(const Unit *unit)
{
	const UnitInfo *ui;
	char buffer[81];

	if (unit == NULL) return;

	ui = &g_table_unitInfo[unit->o.type];

	if (unit->o.type == UNIT_SANDWORM) {
		snprintf(buffer, sizeof(buffer), "%s", String_Get_ByIndex(ui->o.stringID_abbrev));
	} else {
		const char *houseName = g_table_houseInfo[Unit_GetHouseID(unit)].name;
		if (g_table_languageInfo[g_gameConfig.language].noun_before_adj) {
			snprintf(buffer, sizeof(buffer), "%s %s", String_Get_ByIndex(ui->o.stringID_abbrev), houseName);
		} else {
			snprintf(buffer, sizeof(buffer), "%s %s", houseName, String_Get_ByIndex(ui->o.stringID_abbrev));
		}
	}

	if (unit->o.type == UNIT_HARVESTER) {
		uint16 stringID;

		stringID = STR_IS_D_PERCENT_FULL;

		if (unit->actionID == ACTION_HARVEST && unit->amount < 100) {
			uint16 type = Map_GetLandscapeType(Tile_PackTile(unit->o.position));

			if (type == LST_SPICE || type == LST_THICK_SPICE) stringID = STR_IS_D_PERCENT_FULL_AND_HARVESTING;
		}

		if (unit->actionID == ACTION_MOVE && Tools_Index_GetStructure(unit->targetMove) != NULL) {
			stringID = STR_IS_D_PERCENT_FULL_AND_HEADING_BACK;
		} else {
			if (unit->o.script.variables[4] != 0) {
				stringID = STR_IS_D_PERCENT_FULL_AND_AWAITING_PICKUP;
			}
		}

		if (unit->amount == 0) stringID += 4;

		{
			size_t len = strlen(buffer);
			char *s = buffer + len;

			snprintf(s, sizeof(buffer) - len, String_Get_ByIndex(stringID), unit->amount);
		}
	}

	{
		/* add a dot "." at the end of the buffer */
		size_t len = strlen(buffer);
		if (len < sizeof(buffer) - 1) {
			buffer[len] = '.';
			buffer[len + 1] = '\0';
		}
	}
	GUI_DisplayText(buffer, 2);
}

void Unit_DisplayGroupStatusText(void)
{
	if (!Unit_AnySelected())
		return;

	int highest_priority = -1;
	Unit *best_candidate = NULL;
	int iter;
	
	for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
		const UnitInfo *ui = &g_table_unitInfo[u->o.type];
		int priority = ui->o.priorityTarget;

		/* Harvester status text contains important information! */
		if (u->o.type == UNIT_HARVESTER)
			priority = 1000 + u->amount;

		if (highest_priority < priority) {
			highest_priority = priority;
			best_candidate = u;
		}
	}

	if (best_candidate != NULL)
		Unit_DisplayStatusText(best_candidate);
}

/**
 * Hide a unit from the viewport. Happens when a unit enters a structure or
 *  gets picked up by a carry-all.
 *
 * @param unit The Unit to hide.
 */
void Unit_Hide(Unit *unit)
{
	if (unit == NULL) return;

	unit->o.flags.s.bulletIsBig = true;
	Unit_UpdateMap(0, unit);
	unit->o.flags.s.bulletIsBig = false;

	Script_Reset(&unit->o.script, g_scriptUnit);
	Unit_UntargetMe(unit);
	Unit_Unselect(unit);

	unit->o.flags.s.isNotOnMap = true;
	Unit_HouseUnitCount_Remove(unit);
}

/**
 * Call a specified type of unit owned by the house to you.
 *
 * @param type The type of the Unit to find.
 * @param houseID The houseID of the Unit to find.
 * @param target To where the found Unit should move.
 * @param createCarryall Create a carryall if none found.
 * @return The found Unit, or NULL if none found.
 */
Unit *
Unit_CallUnitByType(enum UnitType type, uint8 houseID, uint16 target, bool createCarryall)
{
	PoolFindStruct find;
	Unit *unit = NULL;

	for (Unit *u = Unit_FindFirst(&find, houseID, type);
			u != NULL;
			u = Unit_FindNext(&find)) {
		if (u->o.linkedID != 0xFF) continue;
		if (u->targetMove != 0) continue;
		unit = u;
	}

	if (createCarryall && unit == NULL && type == UNIT_CARRYALL) {
		tile32 position;

		g_validateStrictIfZero++;
		position.x = 0;
		position.y = 0;
		unit = Unit_Create(UNIT_INDEX_INVALID, type, houseID, position, 96);
		g_validateStrictIfZero--;

		if (unit != NULL) unit->o.flags.s.byScenario = true;
	}

	if (unit != NULL) {
		unit->targetMove = target;

		Object_Script_Variable4_Set(&unit->o, target);
	}

	return unit;
}

/**
 * Handles what happens when the given unit enters into the given structure.
 *
 * @param unit The Unit.
 * @param s The Structure.
 */
void Unit_EnterStructure(Unit *unit, Structure *s)
{
	const StructureInfo *si;
	const UnitInfo *ui;

	if (unit == NULL || s == NULL) return;

	ui = &g_table_unitInfo[unit->o.type];
	si = &g_table_structureInfo[s->o.type];

	if (!unit->o.flags.s.allocated || s->o.hitpoints == 0) {
		Unit_Remove(unit);
		return;
	}

	unit->o.seenByHouses |= s->o.seenByHouses;
	Unit_Hide(unit);

	if (House_AreAllied(s->o.houseID, Unit_GetHouseID(unit))) {
		const enum StructureState state
			= si->o.flags.busyStateIsIncoming
			? STRUCTURE_STATE_READY : STRUCTURE_STATE_BUSY;
		Structure_Server_SetState(s, state);

		if (s->o.type == STRUCTURE_REPAIR) {
			uint16 countDown;

			countDown = ((ui->o.hitpoints - unit->o.hitpoints) * 256 / ui->o.hitpoints) * (ui->o.buildTime << 6) / 256;

			if (countDown > 1) {
				s->countDown = countDown;
			} else {
				s->countDown = 1;
			}
			/* ENHANCEMENT -- Units can be ejected prematurely, so don't restore hitpoints just yet. */
			/* unit->o.hitpoints = ui->o.hitpoints; */
			unit->o.flags.s.isSmoking = false;
			unit->spriteOffset = 0;
		}
		unit->o.linkedID = s->o.linkedID;
		s->o.linkedID = unit->o.index & 0xFF;
		return;
	}

	if (unit->o.type == UNIT_SABOTEUR) {
		Structure_Damage(s, 500, 1);
		Unit_Remove(unit);
		return;
	}

	/* Take over the building when low on hitpoints */
	if (s->o.hitpoints < si->o.hitpoints / 4) {
		enum HouseType capturer = Unit_GetHouseID(unit);
		House *h;

		/* ENHANCEMENT -- play structure captured voice. */
		if (enhancement_play_additional_voices) {
			Server_Send_PlayVoice(1 << capturer,
					VOICE_ENEMY_STRUCTURE_CAPTURED);
			Server_Send_PlayVoice(FLAG_HOUSE_ALL & ~(1 << capturer),
					VOICE_HARKONNEN_STRUCTURE_CAPTURED + s->o.houseID);
		}

		h = House_Get_ByIndex(s->o.houseID);
		s->o.houseID = capturer;
		h->structuresBuilt = Structure_GetStructuresBuilt(h);

		/* ENHANCEMENT -- recalculate the power and credits for the house losing the structure. */
		if (g_dune2_enhanced) House_CalculatePowerAndCredit(h);

		h = House_Get_ByIndex(s->o.houseID);
		h->structuresBuilt = Structure_GetStructuresBuilt(h);
		g_factoryWindowTotal = -1;

		if (s->o.linkedID != 0xFF) {
			Unit *u = Unit_Get_ByIndex(s->o.linkedID);
			if (u != NULL) u->o.houseID = Unit_GetHouseID(unit);
		}

		House_CalculatePowerAndCredit(h);
		Structure_UpdateMap(s);

		/* ENHANCEMENT -- When taking over a structure, untarget it. Else you will destroy the structure you just have taken over very easily */
		if (enhancement_fix_firing_logic) Structure_UntargetMe(s);

		/* ENHANCEMENT -- When taking over a structure, unveil the fog around the structure. */
		if (g_dune2_enhanced)
			Structure_RemoveFog(UNVEILCAUSE_STRUCTURE_PLACED, s);
	} else {
		uint16 damage;

		/* ENHANCEMENT -- A bug in OpenDune made soldiers into quite effective engineers. */
		if (enhancement_soldier_engineers) {
			damage = s->o.hitpoints / 2;
		} else {
			damage = min(unit->o.hitpoints * 2, s->o.hitpoints / 2);
		}

		Structure_Damage(s, damage, 1);
	}

	Object_Script_Variable4_Clear(&s->o);

	Unit_Remove(unit);
}

static bool
Unit_StructureInRange(const Unit *unit, const Structure *s, uint16 distance)
{
	const enum StructureLayout layout = g_table_structureInfo[s->o.type].layout;
	tile32 curPosition;

	/* ENHANCEMENT -- units on guard normally only check the distance to
	 *
	 * curPosition.tile = s->o.position.tile + g_table_structure_layoutTileDiff[layout].tile;
	 *
	 * against their weapon range.  This means that they may not fire
	 * at a structure from a given position when on guard command,
	 * whereas they would fire at it when on attack command.
	 */
	if (!enhancement_fix_firing_logic) {
		curPosition.x = s->o.position.x + g_table_structure_layoutTileDiff[layout].x;
		curPosition.y = s->o.position.y + g_table_structure_layoutTileDiff[layout].y;
		return (Tile_GetDistance(unit->o.position, curPosition) <= distance);
	}

	for (int i = 0; i < g_table_structure_layoutSize[layout].height; i++) {
		for (int j = 0; j < g_table_structure_layoutSize[layout].width; j++) {
			curPosition.x = ((s->o.position.x & 0xFF00) + (j << 8)) | 0x80;
			curPosition.y = ((s->o.position.y & 0xFF00) + (i << 8)) | 0x80;

			if (Tile_GetDistance(unit->o.position, curPosition) <= distance)
				return true;
		}
	}

	return false;
}

/**
 * Gets the best target structure for the given unit.
 *
 * @param unit The Unit to get the best target for.
 * @param mode How to determine the best target.
 * @return The best target or NULL if none found.
 */
static const Structure *
Unit_FindBestTargetStructure(Unit *unit, uint16 mode)
{
	const Structure *best = NULL;
	uint16 bestPriority = 0;
	tile32 position;
	uint16 distance;
	PoolFindStruct find;

	if (unit == NULL) return NULL;

	position = Tools_Index_GetTile(unit->originEncoded);
	distance = g_table_unitInfo[unit->o.type].fireDistance << 8;

	for (const Structure *s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
			s != NULL;
			s = Structure_FindNext(&find)) {
		tile32 curPosition;
		uint16 priority;

		if (Structure_SharesPoolElement(s->o.type))
			continue;

		if (mode != 0 && mode != 4) {
			if (mode == 1) {
				if (!Unit_StructureInRange(unit, s, distance)) continue;
			} else {
				if (mode != 2) continue;

				curPosition.x = s->o.position.x + g_table_structure_layoutTileDiff[g_table_structureInfo[s->o.type].layout].x;
				curPosition.y = s->o.position.y + g_table_structure_layoutTileDiff[g_table_structureInfo[s->o.type].layout].y;
				if (Tile_GetDistance(position, curPosition) > distance * 2) continue;
			}
		}

		priority = Unit_GetTargetStructurePriority(unit, s);

		if (priority >= bestPriority) {
			best = s;
			bestPriority = priority;
		}
	}

	if (bestPriority == 0) return NULL;

	return best;
}

/**
 * Get the score of entering this tile from a direction.
 *
 * @param unit The Unit to operate on.
 * @param packed The packed tile.
 * @param direction The direction entering this tile from.
 * @return 256 if tile is not accessable, -1 when it is an accessable structure,
 *   or a score to enter the tile otherwise.
 */
int16 Unit_GetTileEnterScore(Unit *unit, uint16 packed, uint16 orient8)
{
	const UnitInfo *ui;
	Unit *u;
	Structure *s;
	uint16 type;
	uint16 res;

	if (unit == NULL) return 0;

	ui = &g_table_unitInfo[unit->o.type];

	if (!Map_IsValidPosition(packed) && ui->movementType != MOVEMENT_WINGER) return 256;

	u = Unit_Get_ByPackedTile(packed);
	if (u != NULL && u != unit && unit->o.type != UNIT_SANDWORM) {
		if (unit->o.type == UNIT_SABOTEUR && unit->targetMove == Tools_Index_Encode(u->o.index, IT_UNIT)) return 0;

		if (House_AreAllied(Unit_GetHouseID(u), Unit_GetHouseID(unit))) return 256;
		if (g_table_unitInfo[u->o.type].movementType != MOVEMENT_FOOT || (ui->movementType != MOVEMENT_TRACKED && ui->movementType != MOVEMENT_HARVESTER)) return 256;
	}

	s = Structure_Get_ByPackedTile(packed);
	if (s != NULL) {
		res = Unit_IsValidMovementIntoStructure(unit, s);
		if (res == 0) return 256;
		return -res;
	}

	type = Map_GetLandscapeType(packed);

	if (g_dune2_enhanced) {
		res = g_table_landscapeInfo[type].movementSpeed[ui->movementType] * ui->movingSpeedFactor / 256;
	} else {
		res = g_table_landscapeInfo[type].movementSpeed[ui->movementType];
	}

	if (unit->o.type == UNIT_SABOTEUR && type == LST_WALL) {
		if (!House_AreAllied(g_map[packed].houseID, Unit_GetHouseID(unit))) res = 255;
	}

	if (res == 0) return 256;

	/* Check if the unit is travelling diagonally. */
	if ((orient8 & 1) != 0) {
		res -= res / 4 + res / 8;
	}

	/* 'Invert' the speed to get a rough estimate of the time taken. */
	res ^= 0xFF;

	return (int16)res;
}

/**
 * Gets the best target for the given unit.
 *
 * @param unit The Unit to get the best target for.
 * @param mode How to determine the best target.
 * @return The encoded index of the best target or 0 if none found.
 */
uint16 Unit_FindBestTargetEncoded(Unit *unit, uint16 mode)
{
	const Structure *s;
	Unit *target;

	if (unit == NULL) return 0;

	s = NULL;

	if (mode == 4) {
		s = Unit_FindBestTargetStructure(unit, mode);

		if (s != NULL) return Tools_Index_Encode(s->o.index, IT_STRUCTURE);

		target = Unit_FindBestTargetUnit(unit, mode);

		if (target == NULL) return 0;
		return Tools_Index_Encode(target->o.index, IT_UNIT);
	}

	target = Unit_FindBestTargetUnit(unit, mode);

	if (unit->o.type != UNIT_DEVIATOR) s = Unit_FindBestTargetStructure(unit, mode);

	if (target != NULL && s != NULL) {
		uint16 priority;

		priority = Unit_GetTargetUnitPriority(unit, target);

		if (Unit_GetTargetStructurePriority(unit, s) >= priority) return Tools_Index_Encode(s->o.index, IT_STRUCTURE);
		return Tools_Index_Encode(target->o.index, IT_UNIT);
	}

	if (target != NULL) return Tools_Index_Encode(target->o.index, IT_UNIT);
	if (s != NULL) return Tools_Index_Encode(s->o.index, IT_STRUCTURE);

	return 0;
}

/**
 * Check if the Unit belonged the the current human, and do some extra tasks.
 *
 * @param unit The Unit to operate on.
 */
void Unit_RemovePlayer(Unit *unit)
{
	if (unit == NULL) return;
	if (!unit->o.flags.s.allocated) return;

	unit->o.flags.s.allocated = false;
	UnitAI_DetachFromSquad(unit);
	Unit_RemoveFromTeam(unit);

	if (!Unit_IsSelected(unit))
		return;

	Unit_Unselect(unit);

	if ((g_selectionType == SELECTIONTYPE_TARGET) && !Unit_AnySelected()) {
		g_unitActive = NULL;
		g_activeAction = 0xFFFF;

		GUI_ChangeSelectionType(SELECTIONTYPE_STRUCTURE);
	}
}

/**
 * Update the map around the Unit depending on the type (entering tile, leaving, staying).
 * @param type The type of action on the map.
 * @param unit The Unit doing the action.
 */
void Unit_UpdateMap(uint16 type, Unit *unit)
{
	const UnitInfo *ui;
	tile32 position;
	uint16 packed;
	Tile *t;
	uint16 radius;

	if (unit == NULL || unit->o.flags.s.isNotOnMap || !unit->o.flags.s.used) return;

	ui = &g_table_unitInfo[unit->o.type];

	if (ui->movementType == MOVEMENT_WINGER) {
		Map_UpdateAround(g_table_unitInfo[unit->o.type].dimension, unit->o.position, unit, g_functions[0][type]);
		return;
	}

	position = unit->o.position;
	packed = Tile_PackTile(position);
	t = &g_map[packed];

	if ((g_mapVisible[packed].fogOverlayBits != 0xF) || (unit->o.houseID == g_playerHouseID)) {
		Unit_HouseUnitCount_Add(unit, g_playerHouseID);
	} else {
		Unit_HouseUnitCount_Remove(unit);
	}

	if (type == 1) {
		if (unit->o.type != UNIT_SANDWORM) {
			Tile_RemoveFogInRadius(House_GetAllies(Unit_GetHouseID(unit)),
					UNVEILCAUSE_UNIT_UPDATE, position, Unit_GetFogUncoverRadius(unit->o.type, g_table_unitInfo[unit->o.type].o.fogUncoverRadius));
		}

		if (Object_GetByPackedTile(packed) == NULL) {
			t->index = unit->o.index + 1;
			t->hasUnit = true;
		}
	}

	radius = ui->dimension + 3;

	if (unit->o.flags.s.bulletIsBig || unit->o.flags.s.isSmoking || (unit->o.type == UNIT_HARVESTER && unit->actionID == ACTION_HARVEST)) radius = 33;

	Map_UpdateAround(radius, position, unit, g_functions[1][type]);

	if (unit->o.type != UNIT_HARVESTER) return;

	/* The harvester is the only 2x1 unit, so also update tiles in behind us. */
	Map_UpdateAround(radius, unit->targetPreLast, unit, g_functions[1][type]);
	Map_UpdateAround(radius, unit->targetLast, unit, g_functions[1][type]);
}

/**
 * Removes the Unit from the given packed tile.
 *
 * @param unit The Unit to remove.
 * @param packed The packed tile.
 */
void Unit_RemoveFromTile(Unit *unit, uint16 packed)
{
	Tile *t = &g_map[packed];

	if (t->hasUnit && Unit_Get_ByPackedTile(packed) == unit && (packed != Tile_PackTile(unit->currentDestination) || unit->o.flags.s.bulletIsBig)) {
		t->index = 0;
		t->hasUnit = false;
	}
}

void Unit_AddToTile(Unit *unit, uint16 packed)
{
	Map_UnveilTile(Unit_GetHouseID(unit), UNVEILCAUSE_UNIT_UPDATE, packed);
}

/**
 * Get the priority a target structure has for a given unit. The higher the value,
 *  the more serious it should look at the target.
 *
 * @param unit The unit looking at a target.
 * @param target The structure to look at.
 * @return The priority of the target.
 */
uint16 Unit_GetTargetStructurePriority(Unit *unit, const Structure *target)
{
	const StructureInfo *si;
	uint16 priority;
	uint16 distance;

	if (unit == NULL || target == NULL) return 0;

	if (House_AreAllied(Unit_GetHouseID(unit), target->o.houseID)) return 0;
	if ((target->o.seenByHouses & (1 << Unit_GetHouseID(unit))) == 0) return 0;

	si = &g_table_structureInfo[target->o.type];
	priority = si->o.priorityBuild + si->o.priorityTarget;
	distance = Tile_GetDistanceRoundedUp(unit->o.position, target->o.position);
	if (distance != 0) priority /= distance;

	return min(priority, 32000);
}

void
Unit_Server_LaunchHouseMissile(House *h, uint16 packed)
{
	Unit *u = Unit_Get_ByIndex(h->houseMissileID);
	const enum UnitType type = u->o.type;
	tile32 tile;

	tile = Tile_UnpackTile(packed);
	tile = Tile_MoveByRandom(tile, 160, false);
	packed = Tile_PackTile(tile);
	uint16 encoded = Tools_Index_Encode(packed, IT_TILE);

	Unit_Free(u);

	Unit_CreateBullet(h->palacePosition, type, h->index, 500, encoded);

	/* ENHANCEMENT -- In Dune II, you only hear "Missile launched" if
	 * you let the timer run out.  Note: you actually get one second
	 * to choose a target after the narrator says "Missile launched".
	 *
	 * ENHANCEMENT -- allied AI can launch deathhand missiles.
	 */
	enum HouseFlag allies = House_GetAllies(h->index);
	enum HouseFlag enemies = FLAG_HOUSE_ALL & (~allies);

	if (!(enhancement_play_additional_voices && h->houseMissileCountdown > 1))
		allies &= ~(1 << h->index);

	Server_Send_PlayVoice(allies, VOICE_MISSILE_LAUNCHED);
	Server_Send_PlayVoice(enemies, VOICE_WARNING_MISSILE_APPROACHING);

	h->houseMissileID = UNIT_INDEX_INVALID;
	h->houseMissileCountdown = 0;
}

void
Unit_HouseUnitCount_Remove(Unit *u)
{
	assert(u != NULL);

	/* ENHANCEMENT -- In the original game, seenByHouses was only
	 * cleared for houses in the find array, potentially allowing
	 * Fremen to target hidden units.
	 */
	u->o.seenByHouses = 0;
}

/**
 * This unit is about to appear on the map. So add it from the house
 *  statistics about allies/enemies, and do some other logic.
 * @param unit The unit to add.
 * @param houseID The house registering the add.
 */
static void
Unit_Server_HouseUnitCount_Add(Unit *unit, enum HouseType houseID)
{
	const UnitInfo *ui;
	uint16 houseIDBit;
	House *h;

	if (unit == NULL) return;

	ui = &g_table_unitInfo[unit->o.type];
	h = House_Get_ByIndex(houseID);

	if (unit->o.type != UNIT_SANDWORM) {
		houseIDBit = House_GetAllies(houseID);
	} else {
		houseIDBit = (1 << houseID);
	}

	if ((unit->o.seenByHouses & houseIDBit) != 0 && h->flags.isAIActive) {
		unit->o.seenByHouses |= houseIDBit;
		return;
	}

	if (!ui->flags.isNormalUnit && unit->o.type != UNIT_SANDWORM) {
		return;
	}

	if (ui->movementType != MOVEMENT_WINGER) {
		if (!House_AreAllied(houseID, Unit_GetHouseID(unit))) {
			h->flags.isAIActive = true;
			House_Get_ByIndex(Unit_GetHouseID(unit))->flags.isAIActive = true;
		}
	}

	if ((g_host_type != HOSTTYPE_NONE)
	 || (g_host_type == HOSTTYPE_NONE
		 && houseID == g_playerHouseID
		 && g_selectionType != SELECTIONTYPE_MENTAT)) {
		bool playBattleMusic = false;
		enum VoiceID feedbackID = VOICE_INVALID;

		if (unit->o.type == UNIT_SANDWORM) {
			if (h->timerSandwormAttack == 0
					&& (g_host_type == HOSTTYPE_NONE
						|| !House_AreAllied(houseID, Unit_GetHouseID(unit)))) {
				playBattleMusic = true;
				feedbackID = VOICE_WARNING_WORM_SIGN;

				if (g_gameConfig.language == LANGUAGE_ENGLISH) {
					GUI_DisplayHint(houseID,
							STR_HINT_WARNING_SANDWORMS_SHAIHULUD_ROAM_DUNE_DEVOURING_ANYTHING_ON_THE_SAND,
							SHAPE_SANDWORM);
				}

				h->timerSandwormAttack = 8;
			}
		} else if (!House_AreAllied(houseID, Unit_GetHouseID(unit))) {
			Team *t;

			if (h->timerUnitAttack == 0) {
				playBattleMusic = true;

				if (unit->o.type == UNIT_SABOTEUR) {
					feedbackID = VOICE_WARNING_SABOTEUR_APPROACHING;
				} else {
					if (g_scenarioID < 3) {
						PoolFindStruct find;

						const Structure *s = Structure_FindFirst(&find, houseID, STRUCTURE_CONSTRUCTION_YARD);
						if (s != NULL) {
							/* ENHANCEMENT -- Dune2's calculation for the direction is cruder than it needs to be. */
							if (enhancement_fix_enemy_approach_direction_warning) {
								const uint8 orient16 = Orientation_256To16(Tile_GetDirection(s->o.position, unit->o.position));
								const uint8 orient4 = ((orient16 + 1) & 0xF) / 4;

								feedbackID = VOICE_WARNING_ENEMY_UNIT_APPROACHING_FROM_THE_NORTH + orient4;
							} else {
								const uint8 orient8 = Orientation_256To8(Tile_GetDirection(s->o.position, unit->o.position));
								const uint8 orient4 = ((orient8 + 1) & 0x7) / 2;

								feedbackID = VOICE_WARNING_ENEMY_UNIT_APPROACHING_FROM_THE_NORTH + orient4;
							}
						} else {
							feedbackID = VOICE_WARNING_ENEMY_UNIT_APPROACHING;
						}
					} else {
						feedbackID = VOICE_WARNING_HARKONNEN_UNIT_APPROACHING + unit->o.houseID;
					}
				}

				h->timerUnitAttack = 8;
			}

			t = Team_Get_ByIndex(unit->team);
			if (t != NULL) t->script.variables[4] = 1;
		}

		if (playBattleMusic)
			Server_Send_PlayBattleMusic(1 << houseID);

		if (feedbackID != VOICE_INVALID)
			Server_Send_PlayVoice(1 << houseID, feedbackID);
	}

	if (!House_AreAllied(houseID, unit->o.houseID) && unit->actionID == ACTION_AMBUSH)
		Unit_Server_SetAction(unit, ACTION_HUNT);

	if (g_host_type == HOSTTYPE_NONE) {
		if (House_AreAllied(unit->o.houseID, g_playerHouseID)) {
			unit->o.seenByHouses = 0xFF;
		} else {
			unit->o.seenByHouses |= houseIDBit;
		}
	} else {
		unit->o.seenByHouses |= House_GetAllies(houseID) | House_GetAIs();
	}
}

static void
Unit_Client_HouseUnitCount_Add(Unit *unit, enum HouseType houseID)
{
	unit->o.seenByHouses |= (1 << houseID);
}

void
Unit_HouseUnitCount_Add(Unit *unit, uint8 houseID)
{
	if (g_host_type != HOSTTYPE_DEDICATED_CLIENT) {
		Unit_Server_HouseUnitCount_Add(unit, houseID);
	} else {
		Unit_Client_HouseUnitCount_Add(unit, houseID);
	}
}

uint16
Unit_GetFogUncoverRadius(enum UnitType unitType, uint16 fogUncoverRadius)
{
	
	if (enhancement_extend_sight_range) {
		switch(unitType)
		{
			case UNIT_TRIKE:
			case UNIT_QUAD:
			case UNIT_RAIDER_TRIKE:
				return 4;
			default:
				return fogUncoverRadius;
		}
	}

	return fogUncoverRadius;
}