/** @file src/pool/pool_structure.h */

#ifndef POOL_STRUCTURE_H
#define POOL_STRUCTURE_H

#include "enum_house.h"
#include "enum_structure.h"
#include "types.h"
#include "../enhancement.h"

enum {
	STRUCTURE_INDEX_MAX_SOFT    = 79, /* Highest index for normal Structure. */
	STRUCTURE_INDEX_MAX_HARD    = 82, /* Highest index for any Structure. */

	STRUCTURE_INDEX_WALL        = 79, /* Index for walls. */
	STRUCTURE_INDEX_SLAB_2x2    = 80, /* Index for 2x2 slabs. */
	STRUCTURE_INDEX_SLAB_1x1    = 81, /* Index for 1x1 slabs. */

	// Amount to raise indices for enhancement_raise_structure_cap.
	STRUCTURE_INDEX_RAISED_AMOUNT = 100,
	
	STRUCTURE_INDEX_INVALID     = 0xFFFF
};

struct PoolFindStruct;
struct Structure;
struct StructurePool;

extern bool Structure_SharesPoolElement(enum StructureType type);

extern struct Structure *Structure_Get_ByIndex(uint16 index);
extern struct Structure *Structure_FindFirst(struct PoolFindStruct *find, enum HouseType houseID, enum StructureType type);
extern struct Structure *Structure_FindNext(struct PoolFindStruct *find);

extern void Structure_Init(void);
extern void Structure_Recount(void);
extern struct Structure *Structure_Allocate(uint16 index, enum StructureType type);
extern void Structure_Free(struct Structure *s);

extern struct StructurePool *StructurePool_Save(void);
extern void StructurePool_Load(struct StructurePool *pool);
extern uint16 StructurePool_GetIndex(int index);

#endif
