/* ai.c
 *
 * Helper functions for brutal AI.
 */

#include <assert.h>

#include "ai.h"

#include "enhancement.h"
#include "pool/house.h"
#include "pool/pool.h"
#include "pool/structure.h"
#include "pool/unit.h"
#include "structure.h"
#include "tile.h"
#include "tools.h"

static int UnitAI_CountHarvesters(enum HouseType houseID);

/*--------------------------------------------------------------*/

bool
AI_IsBrutalAI(enum HouseType houseID)
{
	return (enhancement_brutal_ai && !House_AreAllied(houseID, g_playerHouseID));
}

/*--------------------------------------------------------------*/

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

	const int harvester_count = UnitAI_CountHarvesters(houseID);

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

		default:
			break;
	}

	return buildable;
}

uint32
StructureAI_FilterBuildOptions_Original(enum StructureType s, enum HouseType houseID, uint32 buildable)
{
	VARIABLE_NOT_USED(houseID);

	switch (s) {
		case STRUCTURE_HEAVY_VEHICLE:
			buildable &= ~(1 << UNIT_HARVESTER);
			buildable &= ~(1 << UNIT_MCV);
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
UnitAI_CountHarvesters(enum HouseType houseID)
{
	const House *h = House_Get_ByIndex(houseID);
	int harvester_count = h->harvestersIncoming;

	/* Count harvesters, including those in production and deviated. */
	for (int i = 0; i < g_unitFindCount; i++) {
		Unit *u = g_unitFindArray[i];

		if (u == NULL)
			continue;

		if ((u->o.houseID == houseID) && (u->o.type == UNIT_HARVESTER))
			harvester_count++;
	}

	return harvester_count;
}
