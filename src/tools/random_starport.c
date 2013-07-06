/**
 * @file src/tools/random_starport.c
 *
 * Borland C/C++ LCG, adapted for starport use.
 */

#include <assert.h>
#include "random_starport.h"

static uint32 s_seed;

void
Random_Starport_Seed(uint16 seed)
{
	s_seed = seed;
}

static int16
Random_Starport_RandomLCG(void)
{
	s_seed = 0x015A4E35 * s_seed + 1;
	return (s_seed >> 16) & 0x7FFF;
}

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
