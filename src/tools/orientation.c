/**
 * @file src/tools/orientation.c
 *
 * Orientation routines.
 */

#include "orientation.h"

uint8
Orientation_256To8(uint8 orient256)
{
	return ((orient256 + 16) & 0xE0) >> 5;
}

uint8
Orientation_256To16(uint8 orient256)
{
	return ((orient256 + 8) & 0xF0) >> 4;
}
