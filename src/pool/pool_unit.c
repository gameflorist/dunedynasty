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

static Unit g_unitArray[UNIT_INDEX_MAX];
struct Unit *g_unitFindArray[UNIT_INDEX_MAX];
uint16 g_unitFindCount;

/**
 * @brief   Get the Unit from the pool with the indicated index.
 */
Unit *
Unit_Get_ByIndex(uint16 index)
{
	assert(index < UNIT_INDEX_MAX);
	return &g_unitArray[index];
}

/**
 * @brief   Start finding Units in g_unitFindArray.
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
 */
void
Unit_Init(void)
{
	memset(g_unitArray, 0, sizeof(g_unitArray));
	memset(g_unitFindArray, 0, sizeof(g_unitFindArray));
	g_unitFindCount = 0;

	/* ENHANCEMENT -- Ensure the index is always valid. */
	for (unsigned int i = 0; i < UNIT_INDEX_MAX; i++) {
		g_unitArray[i].o.index = i;
	}
}

/**
 * @brief   Recount all Units, rebuilding g_unitFindArray.
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

	for (unsigned int index = 0; index < UNIT_INDEX_MAX; index++) {
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

		if (0 < index && index < UNIT_INDEX_MAX) {
			u = Unit_Get_ByIndex(index);
			if (u->o.flags.s.used)
				return NULL;
		}
		else {
			/* Find the first unused index. */
			for (index = ui->indexStart; index <= ui->indexEnd; index++) {
				u = Unit_Get_ByIndex(index);
				if (!u->o.flags.s.used)
					break;
			}
			if (index > ui->indexEnd)
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
