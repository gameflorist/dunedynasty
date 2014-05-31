/** @file src/pool/pool_structure.h */

#ifndef POOL_STRUCTURE_H
#define POOL_STRUCTURE_H

#include "enum_house.h"
#include "enum_structure.h"
#include "types.h"

enum {
	STRUCTURE_INDEX_MAX_SOFT = 79,                          /*!< The highest possible index for normal Structure. */
	STRUCTURE_INDEX_MAX_HARD = 82,                          /*!< The highest possible index for any Structure. */

	STRUCTURE_INDEX_WALL     = 79,                          /*!< All walls are are put under index 79. */
	STRUCTURE_INDEX_SLAB_2x2 = 80,                          /*!< All 2x2 slabs are put under index 80. */
	STRUCTURE_INDEX_SLAB_1x1 = 81,                          /*!< All 1x1 slabs are put under index 81. */

	STRUCTURE_INDEX_INVALID  = 0xFFFF
};

struct PoolFindStruct;
struct Structure;

extern bool Structure_SharesPoolElement(enum StructureType type);

extern struct Structure *Structure_Get_ByIndex(uint16 index);
extern struct Structure *Structure_FindFirst(struct PoolFindStruct *find, enum HouseType houseID, enum StructureType type);
extern struct Structure *Structure_FindNext(struct PoolFindStruct *find);

extern void Structure_Init(void);
extern void Structure_Recount(void);
extern struct Structure *Structure_Allocate(uint16 index, uint8 type);
extern void Structure_Free(struct Structure *s);

#endif
