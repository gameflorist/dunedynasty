/**
 * @file src/pool/pool_structure.c
 *
 * %Structure pool routines.
 *
 * ENHANCEMENT -- In the original game, concrete slabs and walls are
 * all share the same dummy Structure.  However, this caused problems
 * in the original game when there are multiple concrete slabs or
 * walls in production.  As a result, there are quite a few special
 * cases here and wherever Structure_FindFirst/Next is used.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "pool_structure.h"

#include "pool.h"
#include "pool_house.h"
#include "../house.h"
#include "../opendune.h"
#include "../structure.h"
#include "../newui/menubar.h"
#include "../scenario.h"

typedef struct StructurePool {
	Structure pool[STRUCTURE_INDEX_MAX_HARD + STRUCTURE_INDEX_RAISED_AMOUNT];
	Structure *find[STRUCTURE_INDEX_MAX_SOFT + STRUCTURE_INDEX_RAISED_AMOUNT];
	uint16 count;
	bool allocated;
} StructurePool;

/** variable_35F4. */
static Structure s_structureArray[STRUCTURE_INDEX_MAX_HARD + STRUCTURE_INDEX_RAISED_AMOUNT];

/** variable_8622. */
static Structure *s_structureFindArray[STRUCTURE_INDEX_MAX_SOFT + STRUCTURE_INDEX_RAISED_AMOUNT];

/** variable_35F8. */
static uint16 s_structureFindCount;

static StructurePool s_structurePoolBackup;
assert_compile(sizeof(s_structurePoolBackup.pool) == sizeof(s_structureArray));
assert_compile(sizeof(s_structurePoolBackup.find) == sizeof(s_structureFindArray));

/**
 * @brief    Returns true for structure types that share pool elements.
 * @details  Introduced.
 */
bool
Structure_SharesPoolElement(enum StructureType type)
{
	return (type == STRUCTURE_SLAB_1x1
	     || type == STRUCTURE_SLAB_2x2
	     || type == STRUCTURE_WALL);
}

/**
 * @brief   Get the Structure from the pool with the indicated index.
 * @details f__1082_03A1_0023_9F5D.
 */
Structure *
Structure_Get_ByIndex(uint16 index)
{
	if (index >= StructurePool_GetIndex(STRUCTURE_INDEX_MAX_HARD)) {
		GUI_DisplayModalMessage("Savegame was saved with raised structure cap. Enable the enhancement in gameplay options and try again! Game will now exit.", 0xFFFF);
		exit(0);
	}
	assert(index < StructurePool_GetIndex(STRUCTURE_INDEX_MAX_HARD));
	
	return &s_structureArray[index];
}

/**
 * @brief   Start finding Structures in s_structureFindArray.
 * @details 1082_00FD_003A_D7E0 and f__1082_0110_0027_2707.
 *          Removed global find struct for when find=NULL.
 */
Structure *
Structure_FindFirst(PoolFindStruct *find,
		enum HouseType houseID, enum StructureType type)
{
	assert(houseID < HOUSE_NEUTRAL || houseID == HOUSE_INVALID);
	assert(type < STRUCTURE_MAX || type == STRUCTURE_INVALID);

	find->houseID = (houseID < HOUSE_NEUTRAL) ? houseID : HOUSE_INVALID;
	find->type    = (type < STRUCTURE_MAX) ? type : 0xFFFF;
	find->index   = 0xFFFF;

	return Structure_FindNext(find);
}

/**
 * @brief   Continue finding Structures in s_structureFindArray.
 * @details f__1082_013D_0038_4AF1 and f__1082_0155_0020_8556.
 *          Removed global find struct for when find=NULL.
 */
Structure *
Structure_FindNext(PoolFindStruct *find)
{
	if (find->index >= s_structureFindCount + 3 && find->index != 0xFFFF)
		return NULL;

	/* First, go to the next index. */
	find->index++;

	assert(s_structureFindCount <= StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT));
	for (; find->index < s_structureFindCount + 3; find->index++) {
		Structure *s = NULL;

		if (find->index < s_structureFindCount) {
			s = s_structureFindArray[find->index];
		} else if (find->index == s_structureFindCount + 0) {
			s = Structure_Get_ByIndex(StructurePool_GetIndex(STRUCTURE_INDEX_WALL));
			assert(s->o.index == StructurePool_GetIndex(STRUCTURE_INDEX_WALL)
			    && s->o.type == STRUCTURE_WALL);
		} else if (find->index == s_structureFindCount + 1) {
			s = Structure_Get_ByIndex(StructurePool_GetIndex(STRUCTURE_INDEX_SLAB_2x2));
			assert(s->o.index == StructurePool_GetIndex(STRUCTURE_INDEX_SLAB_2x2)
			    && s->o.type == STRUCTURE_SLAB_2x2);
		} else if (find->index == s_structureFindCount + 2) {
			s = Structure_Get_ByIndex(StructurePool_GetIndex(STRUCTURE_INDEX_SLAB_1x1));
			assert(s->o.index == StructurePool_GetIndex(STRUCTURE_INDEX_SLAB_1x1)
			    && s->o.type == STRUCTURE_SLAB_1x1);
		}

		if (s == NULL)
			continue;

		if (s->o.flags.s.isNotOnMap && g_validateStrictIfZero == 0)
			continue;

		if (find->houseID != HOUSE_INVALID && find->houseID != s->o.houseID)
			continue;

		if (find->type != 0xFFFF && find->type != s->o.type)
			continue;

		return s;
	}

	return NULL;
}

/**
 * @brief   Initialise the Structure pool.
 * @details f__1082_0098_001C_39E2.
 */
void
Structure_Init(void)
{
	memset(s_structureArray, 0, sizeof(s_structureArray));
	memset(s_structureFindArray, 0, sizeof(s_structureFindArray));
	s_structureFindCount = 0;

	/* ENHANCEMENT -- Ensure the index is always valid. */
	for (unsigned int i = 0; i < StructurePool_GetIndex(STRUCTURE_INDEX_MAX_HARD); i++) {
		s_structureArray[i].o.index = i;
	}

	Structure_Allocate(0, STRUCTURE_SLAB_1x1);
	Structure_Allocate(0, STRUCTURE_SLAB_2x2);
	Structure_Allocate(0, STRUCTURE_WALL);
}

/**
 * @brief   Recount all Structures, rebuilding s_structureFindArray.
 * @details f__1082_000F_0012_A3C7.
 */
void
Structure_Recount(void)
{
#if 0
	PoolFindStruct find;

	for (House *h = House_FindFirst(&find, HOUSE_INVALID);
			h != NULL;
			h = House_FindNext(&find)) {
		h->unitCount = 0;
	}
#endif

	s_structureFindCount = 0;

	for (unsigned int i = 0; i < StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT); i++) {
		Structure *s = Structure_Get_ByIndex(i);

		if (s->o.flags.s.used) {
			s_structureFindArray[s_structureFindCount] = s;
			s_structureFindCount++;
		}
	}
}

/**
 * @brief   Allocate a Structure.
 * @details f__1082_01E8_0020_FFB9.
 */
Structure *
Structure_Allocate(uint16 index, enum StructureType type)
{
	Structure *s = NULL;
	assert(type < STRUCTURE_MAX);

	switch (type) {
		case STRUCTURE_SLAB_1x1:
			index = StructurePool_GetIndex(STRUCTURE_INDEX_SLAB_1x1);
			s = Structure_Get_ByIndex(index);
			break;

		case STRUCTURE_SLAB_2x2:
			index = StructurePool_GetIndex(STRUCTURE_INDEX_SLAB_2x2);
			s = Structure_Get_ByIndex(index);
			break;

		case STRUCTURE_WALL:
			index = StructurePool_GetIndex(STRUCTURE_INDEX_WALL);
			s = Structure_Get_ByIndex(index);
			break;

		default:
			assert(index < StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT) || index == STRUCTURE_INDEX_INVALID);

			if (index < StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT)) {
				s = Structure_Get_ByIndex(index);
				if (s->o.flags.s.used)
					return NULL;
			} else {
				/* Find the first unused index. */
				for (index = 0; index < StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT); index++) {
					s = Structure_Get_ByIndex(index);
					if (!s->o.flags.s.used)
						break;
				}
				if (index == StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT))
					return NULL;
			}

			assert(s_structureFindCount < StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT));
			s_structureFindArray[s_structureFindCount] = s;
			s_structureFindCount++;
			break;
	}
	assert(s != NULL);

	/* Initialise the Structure. */
	memset(s, 0, sizeof(Structure));
	s->o.index             = index;
	s->o.type              = type;
	s->o.linkedID          = 0xFF;
	s->o.flags.s.used      = true;
	s->o.flags.s.allocated = true;

	return s;
}

/**
 * @brief   Free a Structure.
 * @details f__1082_0325_0018_025E.
 */
void
Structure_Free(Structure *s)
{
	unsigned int i;

	BuildQueue_Free(&s->queue);

	memset(&s->o.flags, 0, sizeof(s->o.flags));

	Script_Reset(&s->o.script, g_scriptStructure);

	if (Structure_SharesPoolElement(s->o.type))
		return;

	/* Find the Structure to remove. */
	assert(s_structureFindCount <= StructurePool_GetIndex(STRUCTURE_INDEX_MAX_SOFT));
	for (i = 0; i < s_structureFindCount; i++) {
		if (s_structureFindArray[i] == s)
			break;
	}

	/* We should always find an entry. */
	assert(i < s_structureFindCount);

	s_structureFindCount--;

	/* If needed, close the gap. */
	if (i < s_structureFindCount) {
		memmove(&s_structureFindArray[i], &s_structureFindArray[i + 1],
				(s_structureFindCount - i) * sizeof(s_structureFindArray[0]));
	}
}

/*--------------------------------------------------------------*/

/**
 * @brief   Saves the StructurePool.
 * @details Introduced for server to generate maps without clobbering
 *          the game state.
 */
StructurePool *
StructurePool_Save(void)
{
	StructurePool *pool = &s_structurePoolBackup;
	assert(!pool->allocated);

	memcpy(pool->pool, s_structureArray, sizeof(s_structureArray));
	memcpy(pool->find, s_structureFindArray, sizeof(s_structureFindArray));
	pool->count = s_structureFindCount;

	pool->allocated = true;
	return pool;
}

/**
 * @brief   Restores the StructurePool and deallocates it.
 * @details Introduced for server to generate maps without clobbering
 *          the game state.
 */
void
StructurePool_Load(StructurePool *pool)
{
	assert(pool->allocated);

	memcpy(s_structureArray, pool->pool, sizeof(s_structureArray));
	memcpy(s_structureFindArray, pool->find, sizeof(s_structureFindArray));
	s_structureFindCount = pool->count;

	pool->allocated = false;
}

uint16
StructurePool_GetIndex(int index)
{
	if (enhancement_raise_structure_cap || g_campaign_selected == CAMPAIGNID_SKIRMISH || g_campaign_selected == CAMPAIGNID_MULTIPLAYER)
		return index + STRUCTURE_INDEX_RAISED_AMOUNT;

	return index;
}