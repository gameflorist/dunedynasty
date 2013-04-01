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
