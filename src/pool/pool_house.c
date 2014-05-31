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

/** variable_35FA. */
static House g_houseArray[HOUSE_INDEX_MAX];

/** variable_87C0. */
static House *g_houseFindArray[HOUSE_INDEX_MAX];

/** variable_35FE. */
static uint16 g_houseFindCount;

/**
 * @brief   Get the House from the pool with the indicated index.
 * @details f__10BE_01AB_002F_0E7B.
 *          Removed test for House->flags.used?
 */
House *
House_Get_ByIndex(uint8 index)
{
	assert(index < HOUSE_INDEX_MAX);
	return &g_houseArray[index];
}

/**
 * @brief   Start finding Houses in g_houseFindArray.
 * @details f__10BE_01E2_0027_6596 and f__10BE_01F5_0014_C21B.
 *          Removed global find struct for when find=NULL.
 */
House *
House_FindFirst(PoolFindStruct *find, enum HouseType houseID)
{
	assert(houseID < HOUSE_MAX || houseID == HOUSE_INVALID);

	/* Note: houseID isn't actually used. */
	find->houseID = (houseID < HOUSE_MAX) ? houseID : HOUSE_INVALID;
	find->type    = 0xFFFF;
	find->index   = 0xFFFF;

	return House_FindNext(find);
}

/**
 * @brief   Continue finding Houses in g_houseFindArray.
 * @details f__10BE_020F_004E_633B and f__10BE_0226_0037_B108.
 *          Removed global find struct for when find=NULL.
 */
House *
House_FindNext(PoolFindStruct *find)
{
	if (find->index >= g_houseFindCount && find->index != 0xFFFF)
		return NULL;

	/* First, go to the next index. */
	find->index++;

	for (; find->index < g_houseFindCount; find->index++) {
		House *h = g_houseFindArray[find->index];
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
	memset(g_houseArray, 0, sizeof(g_houseArray));
	memset(g_houseFindArray, 0, sizeof(g_houseFindArray));
	g_houseFindCount = 0;
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

	g_houseFindArray[g_houseFindCount] = h;
	g_houseFindCount++;

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
	for (i = 0; i < g_houseFindCount; i++) {
		if (g_houseFindArray[i] == h)
			break;
	}

	/* We should always find an entry. */
	assert(i < g_houseFindCount);

	BuildQueue_Free(&h->starportQueue);

	g_houseFindCount--;

	/* If needed, close the gap. */
	if (i < g_houseFindCount) {
		memmove(&g_houseFindArray[i], &g_houseFindArray[i + 1],
				(g_houseFindCount - i) * sizeof(g_houseFindArray[0]));
	}
}
#endif
