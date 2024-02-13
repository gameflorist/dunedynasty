/** @file src/saveload/scenario.c Load/save routines for Scenario. */

#include <assert.h>
#include "saveload.h"
#include "../house.h"
#include "../mods/skirmish.h"
#include "../scenario.h"

static OldScenarioStats s_oldStats;

static const SaveLoadDesc s_saveReinforcement[] = {
	SLD_ENTRY (Reinforcement, SLDT_UINT16, unitID),
	SLD_ENTRY (Reinforcement, SLDT_UINT16, locationID),
	SLD_ENTRY (Reinforcement, SLDT_UINT16, timeLeft),
	SLD_ENTRY (Reinforcement, SLDT_UINT16, timeBetween),
	SLD_ENTRY (Reinforcement, SLDT_UINT16, repeat),
	SLD_END
};

const SaveLoadDesc g_saveScenario[] = {
	SLD_GENTRY(SLDT_UINT16, s_oldStats.score),
	SLD_ENTRY (Scenario, SLDT_UINT16, winFlags),
	SLD_ENTRY (Scenario, SLDT_UINT16, loseFlags),
	SLD_ENTRY (Scenario, SLDT_UINT32, mapSeed),
	SLD_ENTRY (Scenario, SLDT_UINT16, mapScale),
	SLD_ENTRY (Scenario, SLDT_UINT16, timeOut),
	SLD_ARRAY (Scenario, SLDT_UINT8,  pictureBriefing, 14),
	SLD_ARRAY (Scenario, SLDT_UINT8,  pictureWin,      14),
	SLD_ARRAY (Scenario, SLDT_UINT8,  pictureLose,     14),
	SLD_GENTRY(SLDT_UINT16, s_oldStats.killedAllied),
	SLD_GENTRY(SLDT_UINT16, s_oldStats.killedEnemy),
	SLD_GENTRY(SLDT_UINT16, s_oldStats.destroyedAllied),
	SLD_GENTRY(SLDT_UINT16, s_oldStats.destroyedEnemy),
	SLD_GENTRY(SLDT_UINT16, s_oldStats.harvestedAllied),
	SLD_GENTRY(SLDT_UINT16, s_oldStats.harvestedEnemy),
	SLD_SLD2  (Scenario,              reinforcement, s_saveReinforcement, 16),
	SLD_END
};

static const SaveLoadDesc s_saveScenario3[] = {
	SLD_ARRAY(Scenario, SLDT_UINT32, score,          HOUSE_NEUTRAL),
	SLD_ARRAY(Scenario, SLDT_UINT16, unitsLost,      HOUSE_NEUTRAL),
	SLD_ARRAY(Scenario, SLDT_UINT16, structuresLost, HOUSE_NEUTRAL),
	SLD_ARRAY(Scenario, SLDT_UINT16, spiceHarvested, HOUSE_NEUTRAL),
	SLD_END
};

/*--------------------------------------------------------------*/

void
Scenario_Load_OldStats(void)
{
	enum HouseType human = g_playerHouseID;
	enum HouseType enemy = HOUSE_INVALID;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		if (!House_AreAllied(human, h)) {
			enemy = h;
			break;
		}
	}
	assert(enemy != HOUSE_INVALID);

	g_scenario.score[human] = s_oldStats.score;
	g_scenario.unitsLost[human] = s_oldStats.killedAllied;
	g_scenario.unitsLost[enemy] = s_oldStats.killedEnemy;
	g_scenario.structuresLost[human] = s_oldStats.destroyedAllied;
	g_scenario.structuresLost[enemy] = s_oldStats.destroyedEnemy;
	g_scenario.spiceHarvested[human] = s_oldStats.harvestedAllied;
	g_scenario.spiceHarvested[enemy] = s_oldStats.harvestedEnemy;
}

void
Scenario_Save_OldStats(void)
{
	Scenario_GetOldStats(g_playerHouseID, &s_oldStats);
}

/*--------------------------------------------------------------*/

bool
Scenario_Load2(FILE *fp, uint32 length)
{
	if (length != HOUSE_NEUTRAL)
		return false;

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		char c;
		fread(&c, sizeof(char), 1, fp);

		     if (c == ' ') g_skirmish.player_config[h].brain = BRAIN_NONE;
		else if (c == 'H') g_skirmish.player_config[h].brain = BRAIN_HUMAN;
		else if (c == 'C') g_skirmish.player_config[h].brain = BRAIN_CPU;
	}

	return true;
}

bool
Scenario_Save2(FILE *fp)
{
	const char brain_char[3] = { ' ', 'H', 'C' };

	for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_NEUTRAL; h++) {
		char c = brain_char[g_skirmish.player_config[h].brain];
		fwrite(&c, sizeof(char), 1, fp);
	}

	return true;
}

bool
Scenario_Load3(FILE *fp, uint32 length)
{
	if (SaveLoad_GetLength(s_saveScenario3) != length)
		return false;

	return SaveLoad_Load(s_saveScenario3, fp, &g_scenario);
}

bool
Scenario_Save3(FILE *fp)
{
	return SaveLoad_Save(s_saveScenario3, fp, &g_scenario);
}
