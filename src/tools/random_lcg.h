/** @file src/tools/random_lcg.h */

#ifndef TOOLS_RANDOM_LCG_H
#define TOOLS_RANDOM_LCG_H

#include "types.h"

extern void   Tools_RandomLCG_Seed(uint16 seed);
extern uint16 Tools_RandomLCG_Range(uint16 min, uint16 max);

#endif
