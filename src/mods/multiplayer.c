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
	g_multiplayer.starting_army = MAP_STARTING_ARMY_SMALL;
	g_multiplayer.lose_condition = MAP_LOSE_CONDITION_STRUCTURES;
	g_multiplayer.worm_count = MAP_WORM_COUNT_2;

	g_multiplayer.landscape_params.min_spice_fields = 24;
	g_multiplayer.landscape_params.max_spice_fields = 48;
	
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
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
	assert(houseID < HOUSE_NEUTRAL);
	return (g_multiplayer.player_config[houseID].brain == BRAIN_NONE);
}

/*--------------------------------------------------------------*/

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

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		if (g_multiplayer.client[h] == 0 || g_multiplayer.player_config[h].brain != BRAIN_HUMAN)
			continue;

		if (!Skirmish_GenUnitsHuman(h, sd)) {
			g_playerHouseID = playerHouseID;
			g_playerHouse = House_Get_ByIndex(g_playerHouseID);
			return false;
		}
	}

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
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
	for (enum HouseType h1 = HOUSE_HARKONNEN; h1 < HOUSE_NEUTRAL; h1++) {
		if (g_multiplayer.player_config[h1].brain == BRAIN_NONE)
			continue;

		g_table_houseAlliance[h1][h1] = HOUSEALLIANCE_ALLIES;

		for (enum HouseType h2 = h1 + 1; h2 < HOUSE_NEUTRAL; h2++) {
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
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
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
	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
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
