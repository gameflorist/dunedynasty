/** @file src/pool/pool_house.h */

#ifndef POOL_HOUSE_H
#define POOL_HOUSE_H

#include "enum_house.h"
#include "types.h"

struct House;
struct PoolFindStruct;

extern struct House *House_Get_ByIndex(uint8 index);
extern struct House *House_FindFirst(struct PoolFindStruct *find, enum HouseType houseID);
extern struct House *House_FindNext(struct PoolFindStruct *find);

extern void House_Init(void);
extern struct House *House_Allocate(uint8 index);
extern void House_Free(struct House *h);

#endif
