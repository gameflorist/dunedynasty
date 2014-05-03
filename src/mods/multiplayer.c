/* multiplayer.c */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "multiplayer.h"

#include "skirmish.h"
#include "../house.h"
#include "../map.h"
#include "../net/net.h"
#include "../opendune.h"
#include "../pool/house.h"
#include "../scenario.h"
#include "../tools/coord.h"
#include "../unit.h"

Multiplayer g_multiplayer;

bool
Multiplayer_IsHouseAvailable(enum HouseType houseID)
{
	assert(houseID < HOUSE_MAX);
	return (g_multiplayer.client[houseID] == 0);
}

/*--------------------------------------------------------------*/

static bool
Multiplayer_GenUnitsHuman(enum HouseType houseID, struct SkirmishData *sd)
{
	const uint16 start_location = Skirmish_FindStartLocation(houseID, 32, sd);
	if (!Map_IsValidPosition(start_location))
		return false;

	const tile32 position = Tile_UnpackTile(start_location);
	Scenario_Create_Unit(houseID, UNIT_MCV, 256, position, 127,
			g_table_unitInfo[UNIT_MCV].o.actionsPlayer[3]);

	return true;
}

bool
Multiplayer_GenHouses(struct SkirmishData *sd)
{
	const enum HouseType playerHouseID = g_playerHouseID;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_multiplayer.client[h] == 0)
			continue;

		Scenario_Create_House(h, BRAIN_HUMAN, 1000, 0, 25);
		if (!Multiplayer_GenUnitsHuman(h, sd)) {
			g_playerHouseID = playerHouseID;
			g_playerHouse = House_Get_ByIndex(g_playerHouseID);
			return false;
		}
	}

	g_playerHouseID = playerHouseID;
	g_playerHouse = House_Get_ByIndex(g_playerHouseID);
	return true;
}

static void
Multiplayer_Prepare(void)
{
	memset(g_table_houseAlliance, 0, sizeof(g_table_houseAlliance));

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		g_table_houseAlliance[h][h] = HOUSEALLIANCE_ALLIES;
	}
}

bool
Multiplayer_GenerateMap(bool newseed)
{
	assert(g_campaign_selected == CAMPAIGNID_MULTIPLAYER);

	g_campaignID = 9;
	g_scenarioID = 22;

	if (newseed) {
		/* DuneMaps only supports 15 bit maps seeds, so there. */
		g_multiplayer.seed = rand() & 0x7FFF;
	}

	bool is_playable = true;
	if (is_playable) {
		Campaign_Load();
		Multiplayer_Prepare();
	}

	return Skirmish_GenerateMap1(is_playable);
}
