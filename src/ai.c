/* ai.c
 *
 * Helper functions for brutal AI.
 */

#include <assert.h>
#include <math.h>
#include "os/math.h"

#include "ai.h"

#include "enhancement.h"
#include "map.h"
#include "pool/house.h"
#include "pool/pool.h"
#include "pool/structure.h"
#include "pool/unit.h"
#include "scenario.h"
#include "structure.h"
#include "team.h"
#include "tile.h"
#include "tools.h"

#ifndef M_PI
# define M_PI (3.14159265358979323846)
#endif

enum AISquadState {
	AISQUAD_RECRUITING,
	AISQUAD_ASSEMBLE_SQUAD,
	AISQUAD_DETOUR1,
	AISQUAD_DETOUR2,
	AISQUAD_DETOUR3,
	AISQUAD_DISBAND
};

typedef struct AISquad {
	enum SquadID aiSquad;
	enum AISquadState state;
	enum HouseType houseID;
	int num_members;
	int max_members;
	uint16 waypoint[5];
} AISquad;

static AISquad s_aisquad[SQUADID_MAX + 1];

static int UnitAI_CountUnits(enum HouseType houseID, enum UnitType unit_type);

/*--------------------------------------------------------------*/

bool
AI_IsBrutalAI(enum HouseType houseID)
{
	return (enhancement_brutal_ai && !House_AreAllied(houseID, g_playerHouseID));
}

/*--------------------------------------------------------------*/

static bool
StructureAI_ShouldBuildCarryalls(enum HouseType houseID)
{
	const int carryall_count = UnitAI_CountUnits(houseID, UNIT_CARRYALL);
	const int optimal_carryall_count = 2;

	/* Build a second carryall since we have more harvesters, but it
	 * will also help out with repair duty, and serves as a backup.
	 */
	return (optimal_carryall_count > carryall_count);
}

static bool
StructureAI_ShouldBuildHarvesters(enum HouseType houseID)
{
	PoolFindStruct find;

	find.houseID = houseID;
	find.index = 0xFFFF;
	find.type = STRUCTURE_REFINERY;

	Structure *s = Structure_Find(&find);
	int refinery_count = 0;

	while (s != NULL) {
		refinery_count++;
		s = Structure_Find(&find);
	}

	const int harvester_count = UnitAI_CountUnits(houseID, UNIT_HARVESTER);

	/* If no harvesters, wait for the gifted harvester. */
	if (harvester_count == 0)
		return false;

	const int optimal_harvester_count =
		(refinery_count == 0) ? 0 :
		(refinery_count == 1) ? 2 : 3;

	return (optimal_harvester_count > harvester_count);
}

uint32
StructureAI_FilterBuildOptions(enum StructureType s, enum HouseType houseID, uint32 buildable)
{
	switch (s) {
		case STRUCTURE_HEAVY_VEHICLE:
			if (!StructureAI_ShouldBuildHarvesters(houseID))
				buildable &= ~(1 << UNIT_HARVESTER);

			buildable &= ~(1 << UNIT_MCV);
			break;

		case STRUCTURE_HIGH_TECH:
			if (!StructureAI_ShouldBuildCarryalls(houseID))
				buildable &= ~(1 << UNIT_CARRYALL);
			break;

		default:
			break;
	}

	return buildable;
}

uint32
StructureAI_FilterBuildOptions_Original(enum StructureType s, enum HouseType houseID, uint32 buildable)
{
	PoolFindStruct find;

	switch (s) {
		case STRUCTURE_HEAVY_VEHICLE:
			buildable &= ~(1 << UNIT_HARVESTER);
			buildable &= ~(1 << UNIT_MCV);
			break;

		case STRUCTURE_HIGH_TECH:
			find.houseID = houseID;
			find.index   = 0xFFFF;
			find.type    = UNIT_CARRYALL;

			if (Unit_Find(&find))
				buildable &= ~(1 << UNIT_CARRYALL);
			break;

		default:
			break;
	}

	return buildable;
}

/*--------------------------------------------------------------*/

bool
UnitAI_CallCarryallToEvadeSandworm(const Unit *harvester)
{
	/* Already linked. */
	if (harvester->o.script.variables[4] != 0)
		return true;

	PoolFindStruct find;
	Unit *sandworm;
	bool sandworm_is_close = false;

	find.houseID = HOUSE_INVALID;
	find.type = UNIT_SANDWORM;
	find.index = 0xFFFF;

	sandworm = Unit_Find(&find);
	while (sandworm != NULL) {
		const uint16 distance = Tile_GetDistanceRoundedUp(harvester->o.position, sandworm->o.position);

		if (distance <= 5) {
			sandworm_is_close = true;
			break;
		}

		sandworm = Unit_Find(&find);
	}

	if (!sandworm_is_close)
		return false;

	/* Script_Unit_CallUnitByType, without the stack peek. */
	uint16 encoded;
	uint16 encoded2;
	Unit *carryall;

	encoded = Tools_Index_Encode(harvester->o.index, IT_UNIT);
	carryall = Unit_CallUnitByType(UNIT_CARRYALL, Unit_GetHouseID(harvester), encoded, false);
	if (carryall == NULL)
		return false;

	encoded2 = Tools_Index_Encode(carryall->o.index, IT_UNIT);
	Object_Script_Variable4_Link(encoded, encoded2);
	carryall->targetMove = encoded;
	return true;
}

static int
UnitAI_CountUnits(enum HouseType houseID, enum UnitType unit_type)
{
	const House *h = House_Get_ByIndex(houseID);
	int unit_count = 0;

	if (unit_type == UNIT_HARVESTER)
		unit_count = h->harvestersIncoming;

	/* Count units, including units in production and units deviated. */
	for (int i = 0; i < g_unitFindCount; i++) {
		Unit *u = g_unitFindArray[i];

		if (u == NULL)
			continue;

		if ((u->o.houseID == houseID) && (u->o.type == unit_type))
			unit_count++;
	}

	return unit_count;
}

bool
UnitAI_ShouldDestructDevastator(const Unit *devastator)
{
	if (devastator->o.type != UNIT_DEVASTATOR)
		return false;

	int x = Tile_GetPosX(devastator->o.position);
	int y = Tile_GetPosY(devastator->o.position);
	int net_damage = 0;

	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			if (!((0 <= x + dx && x + dx < MAP_SIZE_MAX) && (0 <= y + dy && y + dy < MAP_SIZE_MAX)))
				continue;

			uint16 packed = Tile_PackXY(x + dx, y + dy);
			Unit *u = Unit_Get_ByPackedTile(packed);

			if (u == NULL)
				continue;

			int cost = g_table_unitInfo[u->o.type].o.buildCredits;

			if (House_AreAllied(devastator->o.houseID, u->o.houseID)) {
				net_damage += cost;
			}
			else {
				net_damage -= cost;
			}
		}
	}

	return (net_damage > 0);
}

/*--------------------------------------------------------------*/

void
UnitAI_ClearSquads(void)
{
	for (enum SquadID aiSquad = SQUADID_1; aiSquad <= SQUADID_MAX; aiSquad++) {
		s_aisquad[aiSquad].num_members = 0;
	}
}

static void
UnitAI_ClampWaypoint(int *x, int *y)
{
	const MapInfo *mapInfo = &g_mapInfos[g_scenario.mapScale];

	*x = clamp(mapInfo->minX, *x, mapInfo->minX + mapInfo->sizeX - 1);
	*y = clamp(mapInfo->minY, *y, mapInfo->minY + mapInfo->sizeY - 1);
}

static void
UnitAI_SquadPlotWaypoints(AISquad *squad, Unit *unit, uint16 target_encoded)
{
	uint16 origin = Tile_PackTile(unit->o.position);
	uint16 target = Tools_Index_GetPackedTile(target_encoded);

	int originx = Tile_GetPackedX(origin);
	int originy = Tile_GetPackedY(origin);
	int targetx = Tile_GetPackedX(target);
	int targety = Tile_GetPackedY(target);

	float dx = originx - targetx;
	float dy = targety - originy;
	float theta = atan2f(dy, dx);

	int detourx1 = targetx + 48.0f * cos(theta + M_PI * 60.0f/180.0f);
	int detoury1 = targety - 48.0f * sin(theta + M_PI * 60.0f/180.0f);
	int detourx2 = targetx + 32.0f * cos(theta + M_PI * 120.0f/180.0f);
	int detoury2 = targety - 32.0f * sin(theta + M_PI * 120.0f/180.0f);
	int detourx3 = targetx + 16.0f * cos(theta + M_PI * 180.0f/180.0f);
	int detoury3 = targety - 16.0f * sin(theta + M_PI * 180.0f/180.0f);

	UnitAI_ClampWaypoint(&detourx1, &detoury1);
	UnitAI_ClampWaypoint(&detourx2, &detoury2);
	UnitAI_ClampWaypoint(&detourx3, &detoury3);

	/* Assemble here before the operation. */
	squad->waypoint[0] = Tile_PackTile(unit->o.position);
	squad->waypoint[1] = squad->waypoint[0];

	/* Detours. */
	squad->waypoint[2] = Tile_PackXY(detourx1, detoury1);
	squad->waypoint[3] = Tile_PackXY(detourx2, detoury2);
	squad->waypoint[4] = Tile_PackXY(detourx3, detoury3);
}

static Unit *
UnitAI_SquadFind(const AISquad *squad, PoolFindStruct *find)
{
	Unit *u = Unit_Find(find);

	while (u != NULL) {
		if (u->aiSquad == squad->aiSquad)
			return u;

		u = Unit_Find(find);
	}

	return NULL;
}

static void
UnitAI_AssignSquad(Unit *unit, uint16 destination)
{
	enum SquadID emptySquadID = SQUADID_INVALID;
	int distance;

	if ((unit->actionID != ACTION_HUNT) || (unit->aiSquad != SQUADID_INVALID))
		return;

	/* Only attempt flank attacks with regular tanks. */
	if (!(UNIT_LAUNCHER <= unit->o.type && unit->o.type <= UNIT_QUAD))
		return;

	/* Don't trust deviated units! */
	if (Unit_GetHouseID(unit) != unit->o.houseID)
		return;

	/* If the destination is close, just go. */
	distance = Tile_GetDistanceRoundedUp(unit->o.position, Tools_Index_GetTile(destination));
	if (distance < 32)
		return;

	/* Consider joining a squad. */
	for (enum SquadID aiSquad = SQUADID_1; aiSquad <= SQUADID_MAX; aiSquad++) {
		AISquad *squad = &s_aisquad[aiSquad];

		/* Consider creating a new squad later. */
		if (squad->num_members == 0) {
			if (emptySquadID == SQUADID_INVALID)
				emptySquadID = aiSquad;

			continue;
		}

		/* Only accept units in the same house. */
		if (squad->houseID != unit->o.houseID)
			continue;

		/* Squad not accepting any more units. */
		if (squad->state != AISQUAD_RECRUITING)
			continue;

		/* Squad is too far away. */
		distance = Tile_GetDistanceRoundedUp(unit->o.position, Tile_UnpackTile(squad->waypoint[0]));
		if (distance > 16)
			continue;

		unit->aiSquad = aiSquad;
		squad->num_members++;

		if (squad->num_members >= squad->max_members)
			squad->state++;

		return;
	}

	/* Create new squad and attack plan. */
	if (emptySquadID != SQUADID_INVALID) {
		AISquad *squad = &s_aisquad[emptySquadID];

		unit->aiSquad = emptySquadID;

		squad->aiSquad = emptySquadID;
		squad->state = AISQUAD_RECRUITING;
		squad->houseID = unit->o.houseID;
		squad->num_members = 1;
		squad->max_members = 3;

		UnitAI_SquadPlotWaypoints(squad, unit, destination);
	}
}

void
UnitAI_DetachFromSquad(Unit *unit)
{
	if (unit->aiSquad == SQUADID_INVALID)
		return;

	unit->aiSquad = SQUADID_INVALID;
	s_aisquad[unit->aiSquad].num_members--;
}

static void
UnitAI_DisbandSquad(AISquad *squad)
{
	PoolFindStruct find;

	find.houseID = squad->houseID;
	find.type = 0xFFFF;
	find.index = 0xFFFF;

	Unit *u = UnitAI_SquadFind(squad, &find);
	while (u != NULL) {
		u->aiSquad = SQUADID_INVALID;
		u = UnitAI_SquadFind(squad, &find);
	}

	squad->num_members = 0;
}

uint16
UnitAI_GetSquadDestination(Unit *unit, uint16 destination)
{
	/* Consider joining a squad on long journeys. */
	UnitAI_AssignSquad(unit, destination);

	if (unit->aiSquad == SQUADID_INVALID)
		return destination;

	AISquad *squad = &s_aisquad[unit->aiSquad];

	if (squad->state <= AISQUAD_DETOUR3) {
		return Tools_Index_Encode(squad->waypoint[squad->state], IT_TILE);
	}
	else {
		return destination;
	}
}

static bool
UnitAI_SquadIsGathered(const AISquad *squad)
{
	PoolFindStruct find;
	tile32 destination;

	if (squad->state > AISQUAD_DETOUR3)
		return true;

	find.houseID = squad->houseID;
	find.type = 0xFFFF;
	find.index = 0xFFFF;
	destination = Tile_UnpackTile(squad->waypoint[squad->state]);

	Unit *u = UnitAI_SquadFind(squad, &find);
	while (u != NULL) {
		int dist = Tile_GetDistanceRoundedUp(u->o.position, destination);
		if (dist >= 8)
			return false;

		u = UnitAI_SquadFind(squad, &find);
	}

	return true;
}

void
UnitAI_SquadLoop(void)
{
	for (enum SquadID aiSquad = SQUADID_1; aiSquad <= SQUADID_MAX; aiSquad++) {
		AISquad *squad = &s_aisquad[aiSquad];

		if (squad->num_members == 0)
			continue;

		if (squad->state == AISQUAD_RECRUITING)
			continue;

		if (UnitAI_SquadIsGathered(squad)) {
			squad->state++;
		}

		if (squad->state == AISQUAD_DISBAND)
			UnitAI_DisbandSquad(squad);
	}
}
