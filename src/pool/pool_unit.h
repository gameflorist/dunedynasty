/** @file src/pool/pool_unit.h */

#ifndef POOL_UNIT_H
#define POOL_UNIT_H

#include "enum_house.h"
#include "enum_unit.h"
#include "types.h"

enum {
	UNIT_INDEX_MAX      = 102, /* Highest index for any Unit. */

	UNIT_INDEX_INVALID  = 0xFFFF
};

struct PoolFindStruct;
struct Unit;

extern struct Unit *g_unitFindArray[UNIT_INDEX_MAX];
extern uint16 g_unitFindCount;

extern struct Unit *Unit_Get_ByIndex(uint16 index);
extern struct Unit *Unit_FindFirst(struct PoolFindStruct *find, enum HouseType houseID, enum UnitType type);
extern struct Unit *Unit_FindNext(struct PoolFindStruct *find);

extern void Unit_Init(void);
extern void Unit_Recount(void);
extern struct Unit *Unit_Allocate(uint16 index, enum UnitType type, enum HouseType houseID);
extern void Unit_Free(struct Unit *u);

extern struct UnitPool *UnitPool_Save(void);
extern void UnitPool_Load(struct UnitPool *pool);

#endif
