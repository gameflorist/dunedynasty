/** @file src/tools/encoded_index.h */

#ifndef TOOLS_ENCODED_INDEX_H
#define TOOLS_ENCODED_INDEX_H

#include "types.h"

struct Object;
struct Structure;
struct Unit;

extern bool Tools_Index_IsValid(uint16 encoded);
extern enum IndexType Tools_Index_GetType(uint16 encoded);
extern uint16 Tools_Index_Encode(uint16 index, enum IndexType type);
extern uint16 Tools_Index_Decode(uint16 encoded);

extern uint16 Tools_Index_GetPackedTile(uint16 encoded);
extern tile32 Tools_Index_GetTile(uint16 encoded);
extern struct Object *Tools_Index_GetObject(uint16 encoded);
extern struct Structure *Tools_Index_GetStructure(uint16 encoded);
extern struct Unit *Tools_Index_GetUnit(uint16 encoded);

#endif
