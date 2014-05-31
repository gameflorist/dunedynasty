/** @file src/pool/pool.h */

#ifndef POOL_POOL_H
#define POOL_POOL_H

#include "enum_house.h"
#include "types.h"

typedef struct PoolFindStruct {
	/* House to find, or HOUSE_INVALID for all. */
	enum HouseType houseID;

	/* Object types to find, or -1 for all. */
	uint16 type;

	/* Internal index for where the search reached. */
	uint16 index;
} PoolFindStruct;

#endif
