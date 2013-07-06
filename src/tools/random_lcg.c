/**
 * @file src/tools/random_lcg.c
 *
 * Borland C/C++ LCG.
 */

#include "random_lcg.h"

static uint32 s_seed;

void
Tools_RandomLCG_Seed(uint16 seed)
{
	s_seed = seed;
}

static int16
Tools_RandomLCG(void)
{
	s_seed = 0x015A4E35 * s_seed + 1;
	return (s_seed >> 16) & 0x7FFF;
}

uint16
Tools_RandomLCG_Range(uint16 min, uint16 max)
{
	uint16 ret;

	if (min > max) {
		const uint16 temp = min;
		min = max;
		max = temp;
	}

	do {
		const uint16 value
			= (int32)Tools_RandomLCG() * (max - min + 1) / 0x8000
			+ min;

		ret = value;
	} while (ret > max);

	return ret;
}
