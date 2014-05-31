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

static struct House g_houseArray[HOUSE_INDEX_MAX];
static struct House *g_houseFindArray[HOUSE_INDEX_MAX];
static uint16 g_houseFindCount;

/**
 * Get a House from the pool with the indicated index.
 *
 * @param index The index of the House to get.
 * @return The House.
 */
House *House_Get_ByIndex(uint8 index)
{
	assert(index < HOUSE_INDEX_MAX);
	return &g_houseArray[index];
}

/**
 * @brief   Start finding Houses in g_houseFindArray.
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
 */
House *
House_FindNext(PoolFindStruct *find)
{
	if (find->index >= g_houseFindCount && find->index != 0xFFFF) return NULL;
	find->index++; /* First, we always go to the next index */

	for (; find->index < g_houseFindCount; find->index++) {
		House *h = g_houseFindArray[find->index];
		if (h != NULL) return h;
	}

	return NULL;
}

/**
 * Initialize the House array.
 *
 * @param address If non-zero, the new location of the House array.
 */
void House_Init(void)
{
	memset(g_houseArray, 0, sizeof(g_houseArray));
	memset(g_houseFindArray, 0, sizeof(g_houseFindArray));
	g_houseFindCount = 0;
}

/**
 * Allocate a House.
 *
 * @param index The index to use.
 * @return The House allocated, or NULL on failure.
 */
House* House_Allocate(uint8 index)
{
	House *h;

	if (index >= HOUSE_INDEX_MAX) return NULL;

	h = House_Get_ByIndex(index);
	if (h->flags.used) return NULL;

	/* Initialize the House */
	memset(h, 0, sizeof(House));
	h->index            = index;
	h->flags.used       = true;
	h->starportLinkedID = UNIT_INDEX_INVALID;
	h->structureActiveID= STRUCTURE_INDEX_INVALID;
	h->houseMissileID   = UNIT_INDEX_INVALID;

	g_houseFindArray[g_houseFindCount++] = h;

	return h;
}

/**
 * Free a House.
 *
 * @param address The address of the House to free.
 */
void House_Free(House *h)
{
	int i;

	/* Walk the array to find the House we are removing */
	for (i = 0; i < g_houseFindCount; i++) {
		if (g_houseFindArray[i] == h) break;
	}
	assert(i < g_houseFindCount); /* We should always find an entry */

	BuildQueue_Free(&h->starportQueue);

	g_houseFindCount--;

	/* If needed, close the gap */
	if (i == g_houseFindCount) return;
	memmove(&g_houseFindArray[i], &g_houseFindArray[i + 1], (g_houseFindCount - i) * sizeof(g_houseFindArray[0]));
}
