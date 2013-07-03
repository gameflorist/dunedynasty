/** @file src/tools.h Various definitions. */

#ifndef TOOLS_H
#define TOOLS_H

#include "types.h"

/**
 * Types of encoded Index.
 */
enum IndexType {
	IT_NONE      = 0,
	IT_TILE      = 1,
	IT_UNIT      = 2,
	IT_STRUCTURE = 3
};

struct Unit;
struct Structure;
struct Object;

extern uint16 Tools_AdjustToGameSpeed(uint16 normal, uint16 minimum, uint16 maximum, bool inverseSpeed);
extern enum IndexType Tools_Index_GetType(uint16 encoded);
extern uint16 Tools_Index_Decode(uint16 encoded);
extern uint16 Tools_Index_Encode(uint16 index, enum IndexType type);
extern bool Tools_Index_IsValid(uint16 encoded);
extern uint16 Tools_Index_GetPackedTile(uint16 encoded);
extern tile32 Tools_Index_GetTile(uint16 encoded);
extern struct Unit *Tools_Index_GetUnit(uint16 encoded);
extern struct Structure *Tools_Index_GetStructure(uint16 encoded);
extern struct Object *Tools_Index_GetObject(uint16 encoded);

extern void Tools_RandomLCG_Seed(uint16 seed);
extern uint16 Tools_RandomLCG_Range(uint16 min, uint16 max);

#endif /* TOOLS_H */
