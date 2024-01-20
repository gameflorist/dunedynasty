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

	g_multiplayer.credits = 1500;
	g_multiplayer.curr_seed = MapGenerator_PickRandomSeed();
	g_multiplayer.next_seed = g_multiplayer.curr_seed;
	g_multiplayer.seed_mode = MAP_SEED_MODE_RANDOM;
	g_multiplayer.lose_condition = MAP_LOSE_CONDITION_STRUCTURES;

	g_multiplayer.landscape_params.min_spice_fields = 24;
	g_multiplayer.landscape_params.max_spice_fields = 48;
	
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		PlayerConfig pc;
		pc.brain = BRAIN_NONE;
		pc.team = (enum PlayerTeam)((int)h +1);
		pc.houseID = h;
		pc.matchType = MATCHTYPE_MULTIPLAYER;
		g_multiplayer.player_config[h] = pc;
	}
}

bool
Multiplayer_IsHouseAvailable(enum HouseType houseID)
{
	assert(houseID < HOUSE_MAX);
	return (g_multiplayer.player_config[houseID].brain == BRAIN_NONE);
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
	} else if (left_is_mountain && !right_is_mountain) {
		/* Put troopers on mountain (left). */
		delta[1] = 2;
		delta[2] = -2;
	} else if (!left_is_mountain && right_is_mountain) {
		/* Put troopers on mountain (right). */
		delta[1] = -2;
		delta[2] = 2;
	} else {
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
	// If anything goes wrong, we need to restore the original player house.
	const enum HouseType playerHouseID = g_playerHouseID;

	if (!Skirmish_GenHouses(sd)) {
		g_playerHouseID = playerHouseID;
		g_playerHouse = House_Get_ByIndex(g_playerHouseID);
		return false;
	}

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_multiplayer.client[h] == 0 || g_multiplayer.player_config[h].brain != BRAIN_HUMAN)
			continue;

		if (!Multiplayer_GenUnitsHuman(h, sd)) {
			g_playerHouseID = playerHouseID;
			g_playerHouse = House_Get_ByIndex(g_playerHouseID);
			return false;
		}
	}

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_multiplayer.player_config[h].brain == BRAIN_CPU) {
			Skirmish_GenUnitsAI(h);
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

	/* Assign alliances. */
	for (enum HouseType h1 = HOUSE_HARKONNEN; h1 < HOUSE_MAX; h1++) {
		if (g_multiplayer.player_config[h1].brain == BRAIN_NONE)
			continue;

		g_table_houseAlliance[h1][h1] = HOUSEALLIANCE_ALLIES;

		for (enum HouseType h2 = h1 + 1; h2 < HOUSE_MAX; h2++) {
			if (g_multiplayer.player_config[h2].brain == BRAIN_NONE)
				continue;

			if (g_multiplayer.player_config[h1].team == g_multiplayer.player_config[h2].team) {
				g_table_houseAlliance[h1][h2] = HOUSEALLIANCE_ALLIES;
				g_table_houseAlliance[h2][h1] = HOUSEALLIANCE_ALLIES;
			} else {
				g_table_houseAlliance[h1][h2] = HOUSEALLIANCE_ENEMIES;
				g_table_houseAlliance[h2][h1] = HOUSEALLIANCE_ENEMIES;
			}
		}
	}	

	/* Make sure, Fremen and Saboteurs belong to the houses that spawned them. */
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		HouseInfo *hi = &g_table_houseInfo[h];

		if (hi->specialWeapon == HOUSE_WEAPON_FREMEN) {
			hi->superWeapon.fremen.owner = h;
		}
		
		if (hi->specialWeapon == HOUSE_WEAPON_SABOTEUR) {
			hi->superWeapon.saboteur.owner = h;
		}
	}
}

bool Multiplayer_IsAnyHouseLeftPlaying(void)
{
	bool houseLeft = false;
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
		if (g_multiplayer.state[h] == MP_HOUSE_PLAYING)
			houseLeft = true;
	}

	return houseLeft;
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
	} else {
		MapGenerator_LoadWorldState();
	}

	return MapGenerator_TransitionState(mode, success);
}
