/* multiplayer.c */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "../os/common.h"

#include "multiplayer.h"

#include "skirmish.h"
#include "../house.h"
#include "../map.h"
#include "../net/net.h"
#include "../opendune.h"
#include "../pool/pool_house.h"
#include "../scenario.h"
#include "../tools/coord.h"
#include "../tools/random_lcg.h"
#include "../unit.h"
#include "../video/video.h"

Multiplayer g_multiplayer;

void
Multiplayer_Init(void)
{
	memset(&g_multiplayer, 0, sizeof(g_multiplayer));

	g_multiplayer.credits = 1000;
	g_multiplayer.curr_seed = MapGenerator_PickRandomSeed();
	g_multiplayer.next_seed = g_multiplayer.curr_seed;

	g_multiplayer.landscape_params.min_spice_fields = 24;
	g_multiplayer.landscape_params.max_spice_fields = 48;
}

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
	static const enum UnitType type[3] = {
		UNIT_MCV, UNIT_TRIKE, UNIT_TROOPERS
	};

	int delta[3] = { 0, 0, 0 };
	assert_compile(lengthof(delta) == lengthof(type));

	const uint16 start_location = Skirmish_FindStartLocation(houseID, 32, sd);
	if (!Map_IsValidPosition(start_location))
		return false;

	const enum LandscapeType lst_left = Map_GetLandscapeType(start_location - 2);
	const enum LandscapeType lst_right = Map_GetLandscapeType(start_location + 2);
	const bool left_is_mountain = (lst_left == LST_ENTIRELY_MOUNTAIN || lst_left == LST_PARTIAL_MOUNTAIN);
	const bool right_is_mountain = (lst_right == LST_ENTIRELY_MOUNTAIN || lst_right == LST_PARTIAL_MOUNTAIN);

	if (!left_is_mountain && !right_is_mountain) {
		delta[1] = Tools_RandomLCG_Range(0, 1) ? 2 : -2;
		delta[2] = (delta[1] == 2) ? -2 : 2;
	}
	else if (left_is_mountain && !right_is_mountain) {
		/* Put troopers on mountain (left). */
		delta[1] = 2;
		delta[2] = -2;
	}
	else if (!left_is_mountain && right_is_mountain) {
		/* Put troopers on mountain (right). */
		delta[1] = -2;
		delta[2] = 2;
	}
	else {
		/* Rather unlikely scenario, just let the MCV and Trike overlap. */
		delta[2] = Tools_RandomLCG_Range(0, 1) ? 2 : -2;
	}

	for (unsigned int i = 0; i < lengthof(type); i++) {
		const uint16 packed = start_location + delta[i];
		const tile32 position = Tile_UnpackTile(packed);

		Scenario_Create_Unit(houseID, type[i], 256, position, 127,
				g_table_unitInfo[type[i]].o.actionsPlayer[3]);
	}

	return true;
}

bool
Multiplayer_GenHouses(struct SkirmishData *sd)
{
	const enum HouseType playerHouseID = g_playerHouseID;

	/* Note: create all the houses first to avoid Scenario_Create_Unit
	 * thinking that later human houses belong to AIs.
	 */
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_multiplayer.client[h] == 0)
			continue;

		Scenario_Create_House(h, BRAIN_HUMAN, g_multiplayer.credits, 0, 25);
	}

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_multiplayer.client[h] == 0)
			continue;

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

enum MapGeneratorMode
Multiplayer_GenerateMap(enum MapGeneratorMode mode)
{
	bool success;
	assert(g_campaign_selected == CAMPAIGNID_MULTIPLAYER);

	switch (mode) {
		case MAP_GENERATOR_TRY_TEST_ELSE_STOP:
		case MAP_GENERATOR_TRY_TEST_ELSE_RAND:
			break;

		case MAP_GENERATOR_TRY_RAND_ELSE_STOP:
		case MAP_GENERATOR_TRY_RAND_ELSE_RAND:
			g_multiplayer.test_seed = MapGenerator_PickRandomSeed();
			break;

		case MAP_GENERATOR_FINAL:
			g_multiplayer.test_seed = g_multiplayer.next_seed;
			break;

		case MAP_GENERATOR_STOP:
		default:
			return MAP_GENERATOR_STOP;
	}

	if (mode != MAP_GENERATOR_FINAL) {
		MapGenerator_SaveWorldState();
	}

	{
		g_campaignID = 9;
		g_scenarioID = 22;

		bool is_playable = true;
		if (is_playable) {
			Campaign_Load();
			Multiplayer_Prepare();
		}

		success = Skirmish_GenerateMap1(is_playable);

		if (success) {
			g_multiplayer.next_seed = g_multiplayer.test_seed;

			/* Save the minimap for the lobby. */
			Video_DrawMinimap(0, 0, 0, MINIMAP_SAVE);
		}
	}

	if (mode == MAP_GENERATOR_FINAL) {
		g_multiplayer.curr_seed = g_multiplayer.test_seed;
	}
	else {
		MapGenerator_LoadWorldState();
	}

	return MapGenerator_TransitionState(mode, success);
}
