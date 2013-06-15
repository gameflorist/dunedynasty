/* timer.c */

#include <assert.h>
#include "../os/math.h"

#include "timer.h"

#include "../config.h"
#include "../enhancement.h"

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

uint16
Tools_AdjustToGameSpeed(uint16 normal, uint16 minimum, uint16 maximum,
		bool inverseSpeed)
{
	/* ENHANCEMENT -- true game speed achieved by adjusting game timer
	 * tick rate directly.
	 */
	if (enhancement_true_game_speed_adjustment)
		return normal;

	uint16 gameSpeed = g_gameConfig.gameSpeed;

	if (gameSpeed == 2) return normal;
	if (gameSpeed >  4) return normal;

	if (maximum > normal * 2) maximum = normal * 2;
	if (minimum < normal / 2) minimum = normal / 2;

	if (inverseSpeed)
		gameSpeed = 4 - gameSpeed;

	switch (gameSpeed) {
		case 0: return minimum;
		case 1: return normal - (normal - minimum) / 2;
		case 3: return normal + (maximum - normal) / 2;
		case 4: return maximum;
	}

	/* Never reached, but avoids compiler errors */
	return normal;
}

double
Timer_GetUnitMovementFrame(void)
{
	const int duration = 3;
	const int frame = clamp(0, duration + g_timerGame - g_tickUnitMovement, duration - 1);

	return (double)frame / duration;
}

double
Timer_GetUnitRotationFrame(void)
{
	const int duration = Tools_AdjustToGameSpeed(4, 2, 8, true);
	const int frame = clamp(0, duration + g_timerGame - g_tickUnitRotation, duration - 1);

	return (double)frame / duration;
}
