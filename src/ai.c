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
#include "saveload/saveload.h"
#include "scenario.h"
#include "structure.h"
#include "team.h"
#include "tile.h"
#include "timer/timer.h"
#include "tools/coord.h"
#include "tools/encoded_index.h"
#include "tools/orientation.h"
#include "tools/random_general.h"
#include "tools/random_lcg.h"

#ifndef M_PI
# define M_PI (3.14159265358979323846)
#endif

enum AISquadPlanID {
	AISQUAD_DIRECT_A,
	AISQUAD_DIRECT_B,
	AISQUAD_DIRECT_C,
	AISQUAD_DIRECT_D,
	AISQUAD_ENCIRCLE_A, /* 45 */
	AISQUAD_ENCIRCLE_B,

	AISQUAD_FLANK_A,    /* 45, 90 */
	AISQUAD_FLANK_B,
	AISQUAD_135_A,      /* 45, 90, 135 */
	AISQUAD_135_B,
	AISQUAD_BACKSTAB_A, /* 60, 120, 180 */
	AISQUAD_BACKSTAB_B,

	NUM_AISQUAD_ATTACK_PLANS
};

enum AISquadState {
	AISQUAD_RECRUITING,
	AISQUAD_ASSEMBLE_SQUAD,
	AISQUAD_DETOUR1,
	AISQUAD_DETOUR2,
	AISQUAD_DETOUR3,
	AISQUAD_BATTLE_FORMATION,
	AISQUAD_CHARGE,
	AISQUAD_DISBAND
};

typedef struct AISquad {
	enum SquadID aiSquad;
	enum AISquadPlanID plan;
	enum AISquadState state;
	enum HouseType houseID;
	int num_members;
	int max_members;
	uint16 waypoint[5];
	uint16 target;

	int64_t recruitment_timeout;
	int64_t formation_timeout;
} AISquad;

typedef struct AISquadPlan {
	float distance1, angle1;
	float distance2, angle2;
	float distance3, angle3;
} AISquadPlan;

static const AISquadPlan aisquad_attack_plan[NUM_AISQUAD_ATTACK_PLANS] = {
	/* Camp outside of turret range, and assault together. */
	{ 12.0f,   0.0f, 12.0f,    0.0f, 12.0f,    0.0f },
	{ 12.0f,   0.0f, 12.0f,    0.0f, 12.0f,    0.0f },
	{ 15.0f,   0.0f, 15.0f,    0.0f, 15.0f,    0.0f },
	{ 15.0f,   0.0f, 15.0f,    0.0f, 15.0f,    0.0f },
	{ 15.0f,  45.0f, 15.0f,   45.0f, 15.0f,   45.0f },
	{ 15.0f, -45.0f, 15.0f,  -45.0f, 15.0f,  -45.0f },

	{ 25.0f,  45.0f, 25.0f,   90.0f, 15.0f,   90.0f },
	{ 25.0f, -45.0f, 25.0f,  -90.0f, 15.0f,  -90.0f },
	{ 25.0f,  45.0f, 25.0f,   90.0f, 15.0f,  135.0f },
	{ 25.0f, -45.0f, 25.0f,  -90.0f, 15.0f, -135.0f },
	{ 32.0f,  60.0f, 24.0f,  120.0f, 12.0f,  180.0f },
	{ 32.0f, -60.0f, 24.0f, -120.0f, 12.0f, -180.0f }
};

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

	int refinery_count = 0;
	Structure *s;
	while ((s = Structure_Find(&find)) != NULL) {
		refinery_count++;
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

static bool
StructureAI_ShouldBuildInfantry(enum HouseType houseID)
{
	House *h = House_Get_ByIndex(houseID);

	if (h->structuresBuilt & (FLAG_STRUCTURE_HEAVY_VEHICLE | FLAG_STRUCTURE_WOR_TROOPER))
		return false;

	return true;
}

static uint32
StructureAI_FilterBuildOptions(enum StructureType s, enum HouseType houseID, uint32 buildable)
{
	switch (s) {
		case STRUCTURE_HEAVY_VEHICLE:
			if (!StructureAI_ShouldBuildHarvesters(houseID))
				buildable &= ~FLAG_UNIT_HARVESTER;

			buildable &= ~FLAG_UNIT_MCV;
			break;

		case STRUCTURE_HIGH_TECH:
			if (!StructureAI_ShouldBuildCarryalls(houseID))
				buildable &= ~FLAG_UNIT_CARRYALL;
			break;

		case STRUCTURE_BARRACKS:
			if (!StructureAI_ShouldBuildInfantry(houseID))
				buildable &= ~(FLAG_UNIT_INFANTRY | FLAG_UNIT_SOLDIER);
			break;

		default:
			break;
	}

	return buildable;
}

static uint32
StructureAI_FilterBuildOptions_Original(enum StructureType s, enum HouseType houseID, uint32 buildable)
{
	PoolFindStruct find;

	switch (s) {
		case STRUCTURE_HEAVY_VEHICLE:
			buildable &= ~FLAG_UNIT_HARVESTER;
			buildable &= ~FLAG_UNIT_MCV;
			break;

		case STRUCTURE_HIGH_TECH:
			find.houseID = houseID;
			find.index   = 0xFFFF;
			find.type    = UNIT_CARRYALL;

			if (Unit_Find(&find))
				buildable &= ~FLAG_UNIT_CARRYALL;
			break;

		default:
			break;
	}

	return buildable;
}

static int
StructureAI_RemapBuildItem(int index, uint16 *priority)
{
	/* AI builds items like this:
	 * - iterate through possible build units in order.
	 * - the unit has a 25% chance of becoming the thing to build.
	 * - otherwise, build it if it has higher priority.
	 *
	 * Here we change the order of iteration and priorities.
	 */

	const struct {
		uint16 priority;
		enum UnitType unit_type;
	} remap[] = {
		/* Hi-Tech:
		 * Chance of creating carryall even if ornithopter is
		 * available, if required.
		 */
		{  75, UNIT_ORNITHOPTER },
		{  20, UNIT_CARRYALL },     /* at most 1. */

		/* Barracks, WOR:
		 * Always upgrade lone soldiers and troopers to squads, which
		 * are more supply and cost efficient.
		 */
		{  10, UNIT_SOLDIER },
		{  20, UNIT_TROOPER },
		{  20, UNIT_INFANTRY },
		{  50, UNIT_TROOPERS },

		{   0, UNIT_SABOTEUR },     /* never built. */

		/* Heavy factory build ratios:
		 *                  | siege- | IX-tech
		 * 130 siege tank   | 56, 75 | 42, 42, 56
		 *  80 tank         | 19, 25 |  0, 14, 19
		 *  50 deviator     |        |  -,  -, 25
		 * 100 devastator   |        | 33,  -,  -
		 *  60 launcher     | 25,  - | 25,  -,  -
		 *  70 sonic tank   |        |  -, 44,  -
		 *
		 * Also works well for partially upgraded factories.
		 */

		/* General behaviours:
		 *
		 * Before IX-tech, build 75% combat tanks and 25% launchers
		 * when only those two units are available.  Continue to build
		 * launchers after siege tanks are available.
		 *
		 * Harkonnen will phase out combat tanks once devastators are
		 * available.  The backbone will still be siege tanks since
		 * devastators are slow and will destruct.  Continue to use
		 * launchers as they have good firepower.
		 *
		 * Atreides will phase out launchers once sonic tanks are
		 * available, since they fill that role quite well (and the AI
		 * isn't good with launchers).  Continue to build combat tanks
		 * to use as meat, as a 62% sonic tank army is too fragile.
		 *
		 * Ordos will complement their deviators with combat tanks,
		 * which have better speeds than siege tanks.  Not as reliant
		 * on their IX-tech tanks as Harkonnen and Atreides since they
		 * won't win the war.
		 */

		{ 130, UNIT_SIEGE_TANK },
		{  80, UNIT_TANK },
		{  50, UNIT_DEVIATOR },
		{ 100, UNIT_DEVASTATOR },   /* was 175, keep it lower then siege tank. */
		{  60, UNIT_LAUNCHER },     /* was  60, keep it lower than combat tank. */
		{  70, UNIT_SONIC_TANK },   /* was  80, keep it lower than combat tank. */

		{  50, UNIT_TRIKE },
		{  55, UNIT_RAIDER_TRIKE },
		{  60, UNIT_QUAD },

		/* Chance of creating harvester if required. */
		{  10, UNIT_HARVESTER },    /* at most 2 or 3. */

		{  10, UNIT_MCV }           /* never built. */
	};

	if (index > UNIT_MCV) {
		*priority = g_table_unitInfo[index].o.priorityBuild;
		return index;
	}

	*priority = remap[index].priority;
	return remap[index].unit_type;
}

static uint32
StructureAI_GetBuildable(const Structure *s)
{
	uint32 ret = 0;

	if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
		for (int i = STRUCTURE_PALACE; i < STRUCTURE_MAX; i++) {
			if (Structure_GetAvailable(s, i) > 0)
				ret |= (1 << i);
		}
	}
	else if (s->o.type != STRUCTURE_STARPORT) {
		const StructureInfo *si = &g_table_structureInfo[s->o.type];

		for (int i = 0; i < 8; i++) {
			const int u = si->buildableUnits[i];

			if (Structure_GetAvailable(s, u))
				ret |= (1 << u);
		}
	}

	return ret;
}

/**
 * Find the next object to build.
 * @param s The structure in which we can build something.
 * @return The type (either UnitType or StructureType) of what we should build next.
 */
uint16
StructureAI_PickNextToBuild(const Structure *s)
{
	if (s == NULL) return 0xFFFF;

	House *h = House_Get_ByIndex(s->o.houseID);
	uint32 buildable = StructureAI_GetBuildable(s);

	if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
		for (int i = 0; i < 5; i++) {
			uint16 type = h->ai_structureRebuild[i][0];

			if (type == 0) continue;
			if ((buildable & (1 << type)) == 0) continue;

			return type;
		}

		return 0xFFFF;
	}

	if (AI_IsBrutalAI(s->o.houseID)) {
		buildable = StructureAI_FilterBuildOptions(s->o.type, s->o.houseID, buildable);
	}
	else {
		buildable = StructureAI_FilterBuildOptions_Original(s->o.type, s->o.houseID, buildable);
	}

	uint16 type = 0xFFFF;
	uint16 priority_type = 0;
	for (int j = 0; j < UNIT_MAX; j++) {
		uint16 priority_i;
		uint16 i;

		/* Adjustments to build order for brutal AI. */
		if (AI_IsBrutalAI(s->o.houseID)) {
			i = StructureAI_RemapBuildItem(j, &priority_i);
		}
		else {
			i = j;
			priority_i = g_table_unitInfo[i].o.priorityBuild;
		}

		if ((buildable & (1 << i)) == 0) continue;

		if ((Tools_Random_256() % 4) == 0) {
			type = i;
			priority_type = priority_i;
		}

		if (type != 0xFFFF) {
			if (priority_i <= priority_type) continue;
		}

		type = i;
		priority_type = priority_i;
	}

	return type;
}

/*--------------------------------------------------------------*/

uint16
UnitAI_GetAnyEnemyInRange(const Unit *unit)
{
	UnitInfo *ui = &g_table_unitInfo[unit->o.type];
	enum HouseType houseID = Unit_GetHouseID(unit);
	int dist = max(4, ui->fireDistance);

	PoolFindStruct find;

	find.houseID = HOUSE_INVALID;
	find.index = 0xFFFF;
	find.type = 0xFFFF;

	Unit *u = Unit_Find(&find);
	while (u != NULL) {
		if (u->o.type == UNIT_SANDWORM) {
		}
		else if (House_AreAllied(houseID, Unit_GetHouseID(u))) {
		}
		else if (!g_table_unitInfo[u->o.type].flags.isGroundUnit) {
		}
		else if (Tile_GetDistanceRoundedUp(unit->o.position, u->o.position) <= dist) {
			return Tools_Index_Encode(u->o.index, IT_UNIT);
		}

		u = Unit_Find(&find);
	}

	find.houseID = HOUSE_INVALID;
	find.index = 0xFFFF;
	find.type = 0xFFFF;

	Structure *s;
	while ((s = Structure_Find(&find)) != NULL) {
		if (House_AreAllied(houseID, s->o.houseID)) {
		}
		else if (Tile_GetDistanceRoundedUp(unit->o.position, s->o.position) <= dist) {
			return Tools_Index_Encode(s->o.index, IT_STRUCTURE);
		}
	}

	return 0;
}

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

	const int planID = Tools_RandomLCG_Range(0, NUM_AISQUAD_ATTACK_PLANS - 1);
	const AISquadPlan *plan = &aisquad_attack_plan[planID];

	int detourx1 = targetx + plan->distance1 * cos(theta + plan->angle1 * M_PI / 180.0f);
	int detoury1 = targety - plan->distance1 * sin(theta + plan->angle1 * M_PI / 180.0f);
	int detourx2 = targetx + plan->distance2 * cos(theta + plan->angle2 * M_PI / 180.0f);
	int detoury2 = targety - plan->distance2 * sin(theta + plan->angle2 * M_PI / 180.0f);
	int detourx3 = targetx + plan->distance3 * cos(theta + plan->angle3 * M_PI / 180.0f);
	int detoury3 = targety - plan->distance3 * sin(theta + plan->angle3 * M_PI / 180.0f);

	/* Try to disperse to not clog up the factory. */
	originx += Tools_RandomLCG_Range(0, 9) - 5;
	originy += Tools_RandomLCG_Range(0, 9) - 5;

	UnitAI_ClampWaypoint(&originx, &originy);
	UnitAI_ClampWaypoint(&detourx1, &detoury1);
	UnitAI_ClampWaypoint(&detourx2, &detoury2);
	UnitAI_ClampWaypoint(&detourx3, &detoury3);

	squad->plan = planID;

	/* Assemble here before the operation. */
	squad->waypoint[0] = Tile_PackXY(originx, originy);

	if (g_map[squad->waypoint[0]].hasStructure)
		squad->waypoint[0] = Tile_PackTile(unit->o.position);

	squad->waypoint[1] = squad->waypoint[0];

	/* Detours. */
	squad->waypoint[2] = Tile_PackXY(detourx1, detoury1);
	squad->waypoint[3] = Tile_PackXY(detourx2, detoury2);
	squad->waypoint[4] = Tile_PackXY(detourx3, detoury3);

	squad->target = target_encoded;
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

	if ((unit->actionID != ACTION_HUNT) ||
		(unit->aiSquad != SQUADID_INVALID) ||
		Tools_Index_GetType(destination) == IT_UNIT)
		return;

	/* Only attempt flank attacks with regular tanks. */
	if (!(UNIT_LAUNCHER <= unit->o.type && unit->o.type <= UNIT_QUAD))
		return;

	/* Don't trust deviated units! */
	if (Unit_GetHouseID(unit) != unit->o.houseID)
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

	if (emptySquadID == SQUADID_INVALID)
		return;

	/* Consider creating a team if the journey is far. */
	distance = Tile_GetDistanceRoundedUp(unit->o.position, Tools_Index_GetTile(destination));
	if (distance <= 12)
		return;

	/* Create new squad and attack plan. */
	if (Tools_Random_256() & 0x1) {
		AISquad *squad = &s_aisquad[emptySquadID];

		unit->aiSquad = emptySquadID;

		squad->aiSquad = emptySquadID;
		squad->state = AISQUAD_RECRUITING;
		squad->houseID = unit->o.houseID;
		squad->num_members = 1;
		squad->max_members = 3;

		/* 60 ticks per second, distance is roughly 30. */
		squad->recruitment_timeout = g_timerGame + Tools_AdjustToGameSpeed(120 * (distance - 12), 1, 0xFFFF, true);
		UnitAI_SquadPlotWaypoints(squad, unit, destination);
	}
}

static void
UnitAI_SquadCharge(AISquad *squad)
{
	PoolFindStruct find;

	find.houseID = squad->houseID;
	find.type = 0xFFFF;
	find.index = 0xFFFF;

	Unit *u = UnitAI_SquadFind(squad, &find);
	while (u != NULL) {
		Unit_SetAction(u, ACTION_HUNT);
		u->targetAttack = squad->target;

		u = UnitAI_SquadFind(squad, &find);
	}
}

void
UnitAI_DetachFromSquad(Unit *unit)
{
	if (unit->aiSquad == SQUADID_INVALID)
		return;

	if (unit->actionID != ACTION_HUNT)
		Unit_SetAction(unit, ACTION_HUNT);

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
		Unit_SetTarget(u, squad->target);

		u = UnitAI_SquadFind(squad, &find);
	}

	squad->num_members = 0;
}

void
UnitAI_AbortMission(Unit *unit, uint16 enemy)
{
	if (unit->aiSquad == SQUADID_INVALID)
		return;

	AISquad *squad = &s_aisquad[unit->aiSquad];

	if (enemy != 0) {
		squad->target = enemy;
		squad->state = AISQUAD_CHARGE;
	}

	UnitAI_SquadCharge(squad);
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
	else if (squad->state == AISQUAD_BATTLE_FORMATION) {
		return unit->targetMove;
	}
	else {
		return squad->target;
	}
}

static bool
UnitAI_SquadIsInFormation(const AISquad *squad)
{
	PoolFindStruct find;

	if (g_timerGame > squad->formation_timeout)
		return true;

	find.houseID = squad->houseID;
	find.type = 0xFFFF;
	find.index = 0xFFFF;

	Unit *u = UnitAI_SquadFind(squad, &find);
	while (u != NULL) {
		if (u->targetMove != 0)
			return false;

		u = UnitAI_SquadFind(squad, &find);
	}

	return true;
}

static bool
UnitAI_SquadIsGathered(const AISquad *squad)
{
	PoolFindStruct find;
	tile32 destination;

	if (squad->state >= AISQUAD_DISBAND)
		return true;

	if (squad->state <= AISQUAD_DETOUR3) {
		destination = Tile_UnpackTile(squad->waypoint[squad->state]);
	}
	else if (squad->state == AISQUAD_BATTLE_FORMATION) {
		return UnitAI_SquadIsInFormation(squad);
	}
	else {
		uint16 packed = Tools_Index_GetPackedTile(squad->target);
		if (packed == 0)
			return true;

		destination = Tile_UnpackTile(packed);
	}

	find.houseID = squad->houseID;
	find.type = 0xFFFF;
	find.index = 0xFFFF;

	Unit *u = UnitAI_SquadFind(squad, &find);
	while (u != NULL) {
		int dist = Tile_GetDistanceRoundedUp(u->o.position, destination);
		int proximity = max(8, g_table_unitInfo[u->o.type].fireDistance + 2);

		/* AI units on ACTION_HUNT don't usually get closer than their
		 * fire distance.
		 */
		if (dist >= proximity) {
			return false;
		}

		u = UnitAI_SquadFind(squad, &find);
	}

	return true;
}

static void
UnitAI_ArrangeBattleFormation(AISquad *squad)
{
	const int dx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
	const int dy[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

	uint16 curr_packed = squad->waypoint[AISQUAD_DETOUR3];
	int currx = Tile_GetPackedX(curr_packed);
	int curry = Tile_GetPackedY(curr_packed);

	uint16 target_packed = Tools_Index_GetPackedTile(squad->target);
	int targetx = Tile_GetPackedX(target_packed);
	int targety = Tile_GetPackedY(target_packed);

	float theta = atan2f(currx - targetx, targety - curry);
	uint8 orient256 = (int8)(theta * 128.0f / M_PI);
	uint8 orient8 = Orientation_256To8(orient256);
	uint8 tangent8 = (orient8 + 2) & 0x7;
	int distance = aisquad_attack_plan[squad->plan].distance3;
	int rank = 1;
	int sign = 1;

	/* Charge diagonally.  distance = |deltax| * 3 / 2, where |deltax| = |deltay|. */
	if (orient8 & 0x1)
		distance = distance * 2 / 3;

	/* Ideally, unit is positioned here. */
	int ux = targetx + distance * dx[orient8];
	int uy = targety + distance * dy[orient8];

	PoolFindStruct find;

	find.houseID = squad->houseID;
	find.type = 0xFFFF;
	find.index = 0xFFFF;

	Unit *u = UnitAI_SquadFind(squad, &find);
	while (u != NULL) {
		u->targetMove = Tools_Index_Encode(Tile_PackXY(ux, uy), IT_TILE);

		/* We need the destination to be precise! */
		Unit_SetAction(u, ACTION_MOVE);

		ux += sign * rank * dx[tangent8];
		uy += sign * rank * dy[tangent8];
		UnitAI_ClampWaypoint(&ux, &uy);

		rank++;
		sign = -sign;
		u = UnitAI_SquadFind(squad, &find);
	}

	/* Time to build formation, 60 ticks per second. */
	squad->formation_timeout = g_timerGame + Tools_AdjustToGameSpeed(60 * 15, 1, 0xFFFF, true);
}

void
UnitAI_SquadLoop(void)
{
	for (enum SquadID aiSquad = SQUADID_1; aiSquad <= SQUADID_MAX; aiSquad++) {
		AISquad *squad = &s_aisquad[aiSquad];

		if (squad->num_members == 0)
			continue;

		if (squad->state == AISQUAD_RECRUITING) {
			if (g_timerGame > squad->recruitment_timeout)
				squad->state++;

			continue;
		}

		if (UnitAI_SquadIsGathered(squad)) {
			squad->state++;

			if (squad->state == AISQUAD_BATTLE_FORMATION) {
				if (squad->num_members == 1) {
					squad->state++;
				}
				else {
					UnitAI_ArrangeBattleFormation(squad);
				}
			}

			if (squad->state == AISQUAD_CHARGE) {
				UnitAI_SquadCharge(squad);
			}
		}

		if (squad->state == AISQUAD_DISBAND)
			UnitAI_DisbandSquad(squad);
	}
}

/*--------------------------------------------------------------*/

static uint32 SaveLoad_BrutalAI_RecruitmentTimeout(void *object, uint32 value, bool loading);
static uint32 SaveLoad_BrutalAI_FormationTimeout(void *object, uint32 value, bool loading);

static const SaveLoadDesc s_saveBrutalAISquad[] = {
	SLD_ENTRY2(AISquad, SLDT_UINT8,  aiSquad,   SLDT_UINT32),
	SLD_ENTRY2(AISquad, SLDT_UINT8,  plan,      SLDT_UINT32),
	SLD_ENTRY2(AISquad, SLDT_UINT8,  state,     SLDT_UINT32),
	SLD_ENTRY2(AISquad, SLDT_UINT8,  houseID,   SLDT_UINT32),
	SLD_ENTRY (AISquad, SLDT_INT32,  num_members),
	SLD_ENTRY (AISquad, SLDT_INT32,  max_members),
	SLD_ARRAY (AISquad, SLDT_UINT16, waypoint,  5),
	SLD_ENTRY (AISquad, SLDT_UINT16, target),
	SLD_CALLB (AISquad, SLDT_UINT32, recruitment_timeout, SaveLoad_BrutalAI_RecruitmentTimeout),
	SLD_CALLB (AISquad, SLDT_UINT32, formation_timeout,   SaveLoad_BrutalAI_FormationTimeout),
	SLD_END
};
assert_compile(sizeof(enum SquadID) == sizeof(uint32));
assert_compile(sizeof(enum AISquadPlanID) == sizeof(uint32));
assert_compile(sizeof(enum AISquadState) == sizeof(uint32));
assert_compile(sizeof(enum HouseType) == sizeof(uint32));

static uint32
SaveLoad_BrutalAI_RecruitmentTimeout(void *object, uint32 value, bool loading)
{
	AISquad *squad = object;

	if (loading) {
		squad->recruitment_timeout = (value == 0) ? 0 : (g_timerGame + value);
		return 0;
	}
	else {
		if (squad->recruitment_timeout <= g_timerGame) {
			return 0;
		}
		else {
			return squad->recruitment_timeout - g_timerGame;
		}
	}
}

static uint32
SaveLoad_BrutalAI_FormationTimeout(void *object, uint32 value, bool loading)
{
	AISquad *squad = object;

	if (loading) {
		squad->formation_timeout = (value == 0) ? 0 : (g_timerGame + value);
		return 0;
	}
	else {
		if (squad->formation_timeout <= g_timerGame) {
			return 0;
		}
		else {
			return squad->formation_timeout - g_timerGame;
		}
	}
}

bool
BrutalAI_Load(FILE *fp, uint32 length)
{
	for (int i = 0; (i < SQUADID_MAX + 1) && (length > 0); i++) {
		if (!SaveLoad_Load(s_saveBrutalAISquad, fp, &s_aisquad[i]))
			return false;

		length -= SaveLoad_GetLength(s_saveBrutalAISquad);
	}

	return true;
}

bool
BrutalAI_Save(FILE *fp)
{
	for (int i = 0; i < SQUADID_MAX + 1; i++) {
		if (!SaveLoad_Save(s_saveBrutalAISquad, fp, &s_aisquad[i]))
			return false;
	}

	return true;
}
