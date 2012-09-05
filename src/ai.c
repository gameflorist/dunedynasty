/* ai.c
 *
 * Helper functions for brutal AI.
 */

#include <assert.h>

#include "ai.h"

#include "enhancement.h"

bool
AI_IsBrutalAI(enum HouseType houseID)
{
	return (enhancement_brutal_ai && !House_AreAllied(houseID, g_playerHouseID));
}
