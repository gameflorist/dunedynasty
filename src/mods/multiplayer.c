/* multiplayer.c */

#include <assert.h>

#include "multiplayer.h"

Multiplayer g_multiplayer;

bool
Multiplayer_IsHouseAvailable(enum HouseType houseID)
{
	assert(HOUSE_HARKONNEN <= houseID && houseID < HOUSE_MAX);
	return (g_multiplayer.client[houseID] == 0);
}
