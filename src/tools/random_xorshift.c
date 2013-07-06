/**
 * @file src/tools/random_xorshift.c
 *
 * Xorshift RNG, for non-game-mechanics random numbers.
 */

#include <assert.h>
#include "random_xorshift.h"

typedef struct xorshift {
	uint32 x;
	uint32 y;
	uint32 z;
	uint32 w;
} xorshift_t;

static xorshift_t s_xorshift;

/**
 * @brief   Reseeds the xorshift state.
 * @details http://en.wikipedia.org/wiki/Xorshift
 */
void
Random_Xorshift_Seed(uint32 x, uint32 y, uint32 z, uint32 w)
{
	xorshift_t *s = &s_xorshift;

	if (x == 0 && y == 0 && z == 0 && w == 0) {
		x = 123456789;
		y = 362436069;
		z = 521288629;
		w = 88675123;
	}

	s->x = x;
	s->y = y;
	s->z = z;
	s->w = w;
}

/**
 * @brief   Generate a random number.
 * @details http://en.wikipedia.org/wiki/Xorshift
 */
static uint32
xor128(void)
{
	xorshift_t *s = &s_xorshift;
	uint32 t;

	t    = s->x ^ (s->x << 11);
	s->x = s->y;
	s->y = s->z;
	s->z = s->w;
	return s->w = s->w ^ (s->w >> 19) ^ (t ^ (t >> 8));
}

/**
 * @brief   Tools_Random_256, for non-game-mechanics.
 * @details @see Tools_Random_256.
 */
uint8
Random_Xorshift_256(void)
{
	return xor128() & 0xFF;
}

/**
 * @brief   Tools_RandomLCG_Range, for non-game-mechanics.
 * @details @see Tools_RandomLCG_Range.
 */
uint16
Random_Xorshift_Range(uint16 min, uint16 max)
{
	uint16 ret;
	assert(min <= max);

	do {
		const uint16 value
			= (int32)(xor128() & 0x7FFF) * (max - min + 1) / 0x8000
			+ min;

		ret = value;
	} while (ret > max);

	return ret;
}
