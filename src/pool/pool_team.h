/** @file src/pool/pool_team.h */

#ifndef POOL_TEAM_H
#define POOL_TEAM_H

#include "enum_house.h"
#include "types.h"

enum {
	TEAM_INDEX_INVALID = 0xFFFF
};

struct PoolFindStruct;
struct Team;

extern struct Team *Team_Get_ByIndex(uint16 index);
extern struct Team *Team_FindFirst(struct PoolFindStruct *find, enum HouseType houseID);
extern struct Team *Team_FindNext(struct PoolFindStruct *find);

extern void Team_Init(void);
extern void Team_Recount(void);
extern struct Team *Team_Allocate(uint16 index);
extern void Team_Free(struct Team *au);

#endif
