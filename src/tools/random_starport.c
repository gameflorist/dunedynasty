/**
 * @file src/tools/random_starport.c
 *
 * Borland C/C++ LCG, adapted for starport use.
 */

#include <assert.h>
#include "random_starport.h"

#include "../structure.h"
#include "../timer/timer.h"
#include "../unit.h"

static uint16 s_initialSeed;
static uint32 s_seed;

/**
 * @brief   Gets the number of minutes elapsed.
 * @details f__B495_17E6_002B_0A6D, between labels (l__17E6, l__1863).
 */
int64_t
Random_Starport_GetSeedTime(void)
{
	return (g_timerGame - g_tickScenarioStart) / 60 / 60;
}

/**
 * @brief   Gets the starport seed.
 * @details f__B495_17E6_002B_0A6D, between labels (l__17E6, l__1863).
 *          Added scenarioID and houseID as parameters.
 */
uint16
Random_Starport_GetSeed(uint16 scenarioID, enum HouseType houseID)
{
	const uint16 minutes = Random_Starport_GetSeedTime();
	const uint16 seed = minutes + scenarioID + houseID;

	return seed * seed;
}

/**
 * @brief   Restores the starport LCG to the initial state.
 * @details @see Tools_RandomLCG_Seed.
 */
void
Random_Starport_Reseed(void)
{
	s_seed = s_initialSeed;
}

/**
 * @brief   Tools_RandomLCG_Seed, for starport.
 * @details @see Tools_RandomLCG_Seed.
 */
void
Random_Starport_Seed(uint16 seed)
{
	s_initialSeed = seed;
	s_seed = seed;
}

/**
 * @brief   Tools_RandomLCG, for starport.
 * @details @see Tools_RandomLCG.
 */
static int16
Random_Starport_RandomLCG(void)
{
	s_seed = 0x015A4E35 * s_seed + 1;
	return (s_seed >> 16) & 0x7FFF;
}

/**
 * @brief   Tools_RandomLCG_Range, for starport.
 * @details @see Tools_RandomLCG_Range.
 */
static uint16
Random_Starport_Range(uint16 min, uint16 max)
{
	uint16 ret;
	assert(min <= max);

	do {
		const uint16 value
			= (int32)Random_Starport_RandomLCG() * (max - min + 1) / 0x8000
			+ min;

		ret = value;
	} while (ret > max);

	return ret;
}

/**
 * @brief   f__B495_19B0_0016_09F7.
 * @details Exact.
 */
uint16
Random_Starport_CalculatePrice(uint16 credits)
{
	const uint16 rnd
		= Random_Starport_Range(0, 6)
		+ Random_Starport_Range(0, 6);

	credits = (credits / 10) * 4 + (credits / 10) * rnd;
	if (credits > 999)
		credits = 999;

	return credits;
}

/**
 * @brief   Calculate the current price for the unit type.
 * @details Based on f__B495_17E6_002B_0A6D.
 */
uint16
Random_Starport_CalculateUnitPrice(enum UnitType unitType)
{
	Structure s;
	s.o.type = STRUCTURE_STARPORT;

	Random_Starport_Reseed();

	for (enum UnitType u = UNIT_CARRYALL; u < UNIT_MAX; u++) {
		if (Structure_GetAvailable(&s, u) == 0)
			continue;

		if (u == unitType) {
			const ObjectInfo *oi = &g_table_unitInfo[u].o;

			return Random_Starport_CalculatePrice(oi->buildCredits);
		}
		else {
			Random_Starport_Range(0, 6);
			Random_Starport_Range(0, 6);
		}
	}

	return 0xFFFF;
}
