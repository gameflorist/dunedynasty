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
#include "unit.h"

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
