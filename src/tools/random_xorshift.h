/** @file src/tools/random_xorshift.h */

#ifndef TOOLS_RANDOM_XORSHIFT_H
#define TOOLS_RANDOM_XORSHIFT_H

#include "types.h"

extern void   Random_Xorshift_Seed(uint32 x, uint32 y, uint32 z, uint32 w);
extern uint8  Random_Xorshift_256(void);
extern uint16 Random_Xorshift_Range(uint16 min, uint16 max);

#endif
