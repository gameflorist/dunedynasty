/**
 * @file src/tools/random_lcg.c
 *
 * Borland C/C++ LCG.
 */

#include "random_lcg.h"

/** variable_79E4. */
static uint32 s_seed;

/**
 * @brief   f__01F7_07D4_0011_370E.
 * @details Exact: void srand(unsigned seed).
 */
void
Tools_RandomLCG_Seed(uint16 seed)
{
	s_seed = seed;
}

/**
 * @brief   f__01F7_07E5_0011_F68B.
 * @details Exact: int rand(void).
 */
static int16
Tools_RandomLCG(void)
{
	s_seed = 0x015A4E35 * s_seed + 1;
	return (s_seed >> 16) & 0x7FFF;
}

/**
 * @brief   f__2537_000C_001C_86CB.
 * @details Exact.
 */
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
