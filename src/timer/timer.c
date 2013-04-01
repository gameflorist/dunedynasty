/* timer.c */

#include <assert.h>

#include "timer.h"

int64_t g_timerGame;
int64_t g_tickScenarioStart = 0;

int64_t g_tickHousePowerMaintenance = 0;
int64_t g_tickHouseHouse = 0;
int64_t g_tickHouseStarport = 0;
int64_t g_tickHouseReinforcement = 0;
int64_t g_tickHouseMissileCountdown = 0;
int64_t g_tickHouseStarportAvailability = 0;
int64_t g_tickHouseStarportRecalculatePrices = 0;

int64_t g_tickStructureDegrade  = 0;
int64_t g_tickStructureStructure= 0;
int64_t g_tickStructureScript   = 0;
int64_t g_tickStructurePalace   = 0;

int64_t g_tickTeamGameLoop  = 0;

int64_t g_tickUnitMovement  = 0;
int64_t g_tickUnitRotation  = 0;
int64_t g_tickUnitBlinking  = 0;
int64_t g_tickUnitUnknown4  = 0;
int64_t g_tickUnitScript    = 0;
int64_t g_tickUnitUnknown5  = 0;
int64_t g_tickUnitDeviation = 0;

void
Timer_ResetScriptTimers(void)
{
	g_tickHousePowerMaintenance = g_timerGame;
	g_tickHouseHouse = g_timerGame;
	g_tickHouseStarport = g_timerGame;
	g_tickHouseReinforcement = g_timerGame;
	g_tickHouseMissileCountdown = g_timerGame;
	g_tickHouseStarportAvailability = g_timerGame;
	g_tickHouseStarportRecalculatePrices = g_timerGame;

	g_tickStructureDegrade  = g_timerGame;
	g_tickStructureStructure= g_timerGame;
	g_tickStructureScript   = g_timerGame;
	g_tickStructurePalace   = g_timerGame;

	g_tickTeamGameLoop  = g_timerGame;

	g_tickUnitMovement  = g_timerGame;
	g_tickUnitRotation  = g_timerGame;
	g_tickUnitBlinking  = g_timerGame;
	g_tickUnitUnknown4  = g_timerGame;
	g_tickUnitScript    = g_timerGame;
	g_tickUnitUnknown5  = g_timerGame;
	g_tickUnitDeviation = g_timerGame;
}
