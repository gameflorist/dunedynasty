/**
 * @file src/tools/orientation.c
 *
 * Orientation routines.
 */

#include "orientation.h"

/**
 * @brief   f__B4CD_17DC_0019_CB46.
 * @details Removed use of table precomputed by f__B4B8_09D0_0012_0D7D,
 *          between labels (l__09E2, l__0A14).
 */
uint8
Orientation_256To8(uint8 orient256)
{
	return ((orient256 + 16) & 0xE0) >> 5;
}

/**
 * @brief   f__B4CD_17F7_001D_1CA2.
 * @details Removed use of table precomputed by f__B4B8_09D0_0012_0D7D,
 *          between labels (l__09E2, l__0A14).
 */
uint8
Orientation_256To16(uint8 orient256)
{
	return ((orient256 + 8) & 0xF0) >> 4;
}
