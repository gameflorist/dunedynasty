/**
 * @file src/pool/pool_house.c
 *
 * %House pool routines.
 */

#include <assert.h>
#include <string.h>

#include "pool_house.h"

#include "pool.h"
#include "pool_structure.h"
#include "pool_unit.h"
#include "../house.h"

enum {
	HOUSE_INDEX_MAX = HOUSE_MAX
};

typedef struct HousePool {
	House pool[HOUSE_INDEX_MAX];
	House *find[HOUSE_INDEX_MAX];
	uint16 count;
	bool allocated;
} HousePool;

/** variable_35FA. */
static House s_houseArray[HOUSE_INDEX_MAX];

/** variable_87C0. */
static House *s_houseFindArray[HOUSE_INDEX_MAX];

/** variable_35FE. */
static uint16 s_houseFindCount;

static HousePool s_housePoolBackup;
assert_compile(sizeof(s_housePoolBackup.pool) == sizeof(s_houseArray));
assert_compile(sizeof(s_housePoolBackup.find) == sizeof(s_houseFindArray));

/**
 * @brief   Get the House from the pool with the indicated index.
 * @details f__10BE_01AB_002F_0E7B.
 *          Removed test for House->flags.used?
 */
House *
House_Get_ByIndex(uint8 index)
{
	assert(index < HOUSE_INDEX_MAX);
	return &s_houseArray[index];
}

/**
 * @brief   Start finding Houses in s_houseFindArray.
 * @details f__10BE_01E2_0027_6596 and f__10BE_01F5_0014_C21B.
 *          Removed global find struct for when find=NULL.
 */
House *
House_FindFirst(PoolFindStruct *find, enum HouseType houseID)
{
	assert(houseID < HOUSE_NEUTRAL || houseID == HOUSE_INVALID);

	/* Note: houseID isn't actually used. */
	find->houseID = (houseID < HOUSE_NEUTRAL) ? houseID : HOUSE_INVALID;
	find->type    = 0xFFFF;
	find->index   = 0xFFFF;

	return House_FindNext(find);
}

/**
 * @brief   Continue finding Houses in s_houseFindArray.
 * @details f__10BE_020F_004E_633B and f__10BE_0226_0037_B108.
 *          Removed global find struct for when find=NULL.
 */
House *
House_FindNext(PoolFindStruct *find)
{
	if (find->index >= s_houseFindCount && find->index != 0xFFFF)
		return NULL;

	/* First, go to the next index. */
	find->index++;

	for (; find->index < s_houseFindCount; find->index++) {
		House *h = s_houseFindArray[find->index];
		if (h != NULL)
			return h;
	}

	return NULL;
}

/**
 * @brief   Initialise the House pool.
 * @details f__10BE_000C_0020_F10F.
 */
void
House_Init(void)
{
	memset(s_houseArray, 0, sizeof(s_houseArray));
	memset(s_houseFindArray, 0, sizeof(s_houseFindArray));
	s_houseFindCount = 0;

	/* ENHANCEMENT -- Ensure the index is always valid. */
	for (unsigned int i = 0; i < HOUSE_INDEX_MAX; i++) {
		s_houseArray[i].index = i;
	}
}

/**
 * @brief   Allocate a House.
 * @details f__10BE_00A0_0064_DF2A.
 */
House *
House_Allocate(uint8 index)
{
	House *h = House_Get_ByIndex(index);

	if (h->flags.used)
		return NULL;

	/* Initialise the House. */
	memset(h, 0, sizeof(House));
	h->index            = index;
	h->flags.used       = true;
	h->starportLinkedID = UNIT_INDEX_INVALID;

	/* ENHANCEMENT -- Introduced variables. */
	h->structureActiveID= STRUCTURE_INDEX_INVALID;
	h->houseMissileID   = UNIT_INDEX_INVALID;

	s_houseFindArray[s_houseFindCount] = h;
	s_houseFindCount++;

	return h;
}

/**
 * @brief   Free a House.
 * @details Introduced.
 */
#if 0
void
House_Free(House *h)
{
	unsigned int i;

	/* Find the House to remove. */
	for (i = 0; i < s_houseFindCount; i++) {
		if (s_houseFindArray[i] == h)
			break;
	}

	/* We should always find an entry. */
	assert(i < s_houseFindCount);

	BuildQueue_Free(&h->starportQueue);

	s_houseFindCount--;

	/* If needed, close the gap. */
	if (i < s_houseFindCount) {
		memmove(&s_houseFindArray[i], &s_houseFindArray[i + 1],
				(s_houseFindCount - i) * sizeof(s_houseFindArray[0]));
	}
}
#endif

/*--------------------------------------------------------------*/

/**
 * @brief   Saves the HousePool.
 * @details Introduced for server to generate maps without clobbering
 *          the game state.
 */
HousePool *
HousePool_Save(void)
{
	HousePool *pool = &s_housePoolBackup;
	assert(!pool->allocated);

	memcpy(pool->pool, s_houseArray, sizeof(s_houseArray));
	memcpy(pool->find, s_houseFindArray, sizeof(s_houseFindArray));
	pool->count = s_houseFindCount;

	pool->allocated = true;
	return pool;
}

/**
 * @brief   Restores the HousePool and deallocates it.
 * @details Introduced for server to generate maps without clobbering
 *          the game state.
 */
void
HousePool_Load(HousePool *pool)
{
	assert(pool->allocated);

	memcpy(s_houseArray, pool->pool, sizeof(s_houseArray));
	memcpy(s_houseFindArray, pool->find, sizeof(s_houseFindArray));
	s_houseFindCount = pool->count;

	pool->allocated = false;
}
