/**
 * @file src/pool/pool_unit.c
 *
 * %Unit pool routines.
 */

#include <assert.h>
#include <string.h>

#include "pool_unit.h"

#include "pool.h"
#include "pool_house.h"
#include "../house.h"
#include "../opendune.h"
#include "../unit.h"
#include "../scenario.h"

typedef struct UnitPool {
	Unit pool[UNIT_INDEX_MAX_RAISED];
	Unit *find[UNIT_INDEX_MAX_RAISED];
	uint16 count;
	bool allocated;
} UnitPool;

/** variable_35E8. */
static Unit s_unitArray[UNIT_INDEX_MAX_RAISED];

/** variable_843E. */
struct Unit *g_unitFindArray[UNIT_INDEX_MAX_RAISED];

/** variable_35EC. */
uint16 g_unitFindCount;

static UnitPool s_unitPoolBackup;
assert_compile(sizeof(s_unitPoolBackup.pool) == sizeof(s_unitArray));
assert_compile(sizeof(s_unitPoolBackup.find) == sizeof(g_unitFindArray));

/**
 * @brief   Get the Unit from the pool with the indicated index.
 * @details f__0FE4_05FD_002C_15BA.
 */
Unit *
Unit_Get_ByIndex(uint16 index)
{
	assert(index < UnitPool_GetMaxIndex());
	return &s_unitArray[index];
}

/**
 * @brief   Start finding Units in g_unitFindArray.
 * @details f__0FE4_0243_003A_D5F2 and f__0FE4_0256_0027_2707.
 *          Removed global find struct for when find=NULL.
 */
Unit *
Unit_FindFirst(PoolFindStruct *find,
		enum HouseType houseID, enum UnitType type)
{
	assert(houseID < HOUSE_MAX || houseID == HOUSE_INVALID);
	assert(type < UNIT_MAX || type == UNIT_INVALID);

	find->houseID = (houseID < HOUSE_MAX) ? houseID : HOUSE_INVALID;
	find->type    = (type < UNIT_MAX) ? type : 0xFFFF;
	find->index   = 0xFFFF;

	return Unit_FindNext(find);
}

/**
 * @brief   Continue finding Units in g_unitFindArray.
 * @details f__0FE4_0283_0038_4950 and f__0FE4_029B_0020_87FE.
 *          Removed global find struct for when find=NULL.
 */
Unit *
Unit_FindNext(PoolFindStruct *find)
{
	if (find->index >= g_unitFindCount && find->index != 0xFFFF)
		return NULL;

	/* First, go to the next index. */
	find->index++;

	for (; find->index < g_unitFindCount; find->index++) {
		Unit *u = g_unitFindArray[find->index];

		if (u == NULL)
			continue;

		if (u->o.flags.s.isNotOnMap && g_validateStrictIfZero == 0)
			continue;

		if (find->houseID != HOUSE_INVALID && find->houseID != Unit_GetHouseID(u))
			continue;

		if (find->type != 0xFFFF && find->type != u->o.type)
			continue;

		return u;
	}

	return NULL;
}

/**
 * @brief   Initialise the Unit pool.
 * @details f__0FE4_013F_001C_39CA.
 */
void
Unit_Init(void)
{
	memset(s_unitArray, 0, sizeof(s_unitArray));
	memset(g_unitFindArray, 0, sizeof(g_unitFindArray));
	g_unitFindCount = 0;

	/* ENHANCEMENT -- Ensure the index is always valid. */
	for (unsigned int i = 0; i < UnitPool_GetMaxIndex(); i++) {
		s_unitArray[i].o.index = i;
	}
}

/**
 * @brief   Recount all Units, rebuilding g_unitFindArray.
 * @details f__0FE4_018D_0012_A3C7.
 */
void
Unit_Recount(void)
{
	PoolFindStruct find;

	for (House *h = House_FindFirst(&find, HOUSE_INVALID);
			h != NULL;
			h = House_FindNext(&find)) {
		h->unitCount = 0;
	}

	g_unitFindCount = 0;

	for (unsigned int index = 0; index < UnitPool_GetMaxIndex(); index++) {
		Unit *u = Unit_Get_ByIndex(index);

		if (u->o.flags.s.used) {
			House *h = House_Get_ByIndex(u->o.houseID);
			h->unitCount++;

			g_unitFindArray[g_unitFindCount] = u;
			g_unitFindCount++;
		}
	}
}

/**
 * @brief   Allocate a Unit.
 * @details f__0FE4_03A7_0027_85D5.
 */
Unit *
Unit_Allocate(uint16 index, enum UnitType type, enum HouseType houseID)
{
	Unit *u = NULL;
	assert(type < UNIT_MAX);
	assert(houseID < HOUSE_MAX);

	{
		const UnitInfo *ui = &g_table_unitInfo[type];
		House *h = House_Get_ByIndex(houseID);

		if ((g_validateStrictIfZero == 0)
				&& (h->unitCount >= h->unitCountMax)) {
			if (ui->movementType != MOVEMENT_WINGER
			 && ui->movementType != MOVEMENT_SLITHER) {
				return NULL;
			}
		}

		if (0 < index && index < UnitPool_GetMaxIndex()) {
			u = Unit_Get_ByIndex(index);
			if (u->o.flags.s.used)
				return NULL;
		} else {
			/* Find the first unused index. */
			for (index = ui->indexStart; index <= UnitPool_GetIndexEnd(type); index++) {
				u = Unit_Get_ByIndex(index);
				if (!u->o.flags.s.used)
					break;
			}
			if (index > UnitPool_GetIndexEnd(type))
				return NULL;
		}

		h->unitCount++;
	}
	assert(u != NULL);

	/* Initialise the Unit. */
	memset(u, 0, sizeof(Unit));
	u->o.index              = index;
	u->o.type               = type;
	u->o.houseID            = houseID;
	u->o.linkedID           = 0xFF;
	u->o.flags.s.used       = true;
	u->o.flags.s.allocated  = true;
	u->o.flags.s.isUnit     = true;
	u->o.script.delay       = 0;
	u->route[0]             = 0xFF;

	if (type == UNIT_SANDWORM)
		u->amount = 3;

	/* ENHANCEMENT -- Introduced variables. */
	u->permanentFollow      = false;
	u->detonateAtTarget     = false;
	u->deviationDecremented = false;
	u->squadID = SQUADID_INVALID;
	u->aiSquad = SQUADID_INVALID;

	g_unitFindArray[g_unitFindCount] = u;
	g_unitFindCount++;

	return u;
}

/**
 * @brief   Free a Unit.
 * @details f__0FE4_0568_0018_8258.
 */
void
Unit_Free(Unit *u)
{
	unsigned int i;

	memset(&u->o.flags, 0, sizeof(u->o.flags));

	Script_Reset(&u->o.script, g_scriptUnit);

	/* Find the Unit to remove. */
	for (i = 0; i < g_unitFindCount; i++) {
		if (g_unitFindArray[i] == u)
			break;
	}

	/* We should always find an entry. */
	assert(i < g_unitFindCount);

	g_unitFindCount--;

	House *h = House_Get_ByIndex(u->o.houseID);
	h->unitCount--;

	/* If needed, close the gap. */
	if (i < g_unitFindCount) {
		memmove(&g_unitFindArray[i], &g_unitFindArray[i + 1],
				(g_unitFindCount - i) * sizeof(g_unitFindArray[0]));
	}
}

/*--------------------------------------------------------------*/

/**
 * @brief   Saves the UnitPool.
 * @details Introduced for server to generate maps without clobbering
 *          the game state.
 */
UnitPool *
UnitPool_Save(void)
{
	UnitPool *pool = &s_unitPoolBackup;
	assert(!pool->allocated);

	memcpy(pool->pool, s_unitArray, sizeof(s_unitArray));
	memcpy(pool->find, g_unitFindArray, sizeof(g_unitFindArray));
	pool->count = g_unitFindCount;

	pool->allocated = true;
	return pool;
}

/**
 * @brief   Restores the UnitPool and deallocates it.
 * @details Introduced for server to generate maps without clobbering
 *          the game state.
 */
void
UnitPool_Load(UnitPool *pool)
{
	assert(pool->allocated);

	memcpy(s_unitArray, pool->pool, sizeof(s_unitArray));
	memcpy(g_unitFindArray, pool->find, sizeof(g_unitFindArray));
	g_unitFindCount = pool->count;

	pool->allocated = false;
}

/**
 * @brief   Get maximum unit index.
 * @details Introduced for raise_unit_cap.
 */
int
UnitPool_GetMaxIndex()
{
	if (enhancement_raise_unit_cap || g_campaign_selected == CAMPAIGNID_SKIRMISH || g_campaign_selected == CAMPAIGNID_MULTIPLAYER)
		return UNIT_INDEX_MAX_RAISED;

	return UNIT_INDEX_MAX;
}

/**
 * @brief   Get maximum unit index.
 * @details Introduced for raise_unit_cap.
 */
int
UnitPool_GetIndexEnd(enum UnitType type)
{
	const UnitInfo *ui = &g_table_unitInfo[type];
	if (enhancement_raise_unit_cap || g_campaign_selected == CAMPAIGNID_SKIRMISH || g_campaign_selected == CAMPAIGNID_MULTIPLAYER) {
		if (type >= UNIT_INFANTRY && type <= UNIT_MCV && type != UNIT_SABOTEUR)
			return UNIT_INDEX_MAX_RAISED - 1;
	}
	return ui->indexEnd;
}