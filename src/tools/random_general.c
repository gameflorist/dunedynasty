/**
 * @file src/tools/random_general.c
 *
 * General RNG, used during map creation in particular.
 */

#include "random_general.h"

static uint8 s_seed[4];

void
Tools_Random_Seed(uint32 seed)
{
	s_seed[0] = (seed >>  0) & 0xFF;
	s_seed[1] = (seed >>  8) & 0xFF;
	s_seed[2] = (seed >> 16) & 0xFF;
	s_seed[3] = (seed >> 24) & 0xFF;
}

uint8
Tools_Random_256(void)
{
	const uint8 carry0 = (s_seed[0] >> 1) & 0x01;
	const uint8 carry2 = (s_seed[2] >> 7);
	s_seed[2] = (s_seed[2] << 1) | carry0;

	const uint8 carry1 = (s_seed[1] >> 7);
	s_seed[1] = (s_seed[1] << 1) | carry2;

	const uint8 carry = ((s_seed[0] >> 2) - s_seed[0] - (carry1 ^ 0x01)) & 0x01;
	s_seed[0] = (carry << 7) | (s_seed[0] >> 1);

	return s_seed[0] ^ s_seed[1];
}
