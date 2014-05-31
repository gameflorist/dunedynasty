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

#include "pool_structure.h"

#include "pool.h"
#include "pool_house.h"
#include "../house.h"
#include "../opendune.h"
#include "../structure.h"

static Structure g_structureArray[STRUCTURE_INDEX_MAX_HARD];
static Structure *g_structureFindArray[STRUCTURE_INDEX_MAX_SOFT];
static uint16 g_structureFindCount;

/**
 * @brief    Returns true for structure types that share pool elements.
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
 */
Structure *
Structure_Get_ByIndex(uint16 index)
{
	assert(index < STRUCTURE_INDEX_MAX_HARD);
	return &g_structureArray[index];
}

/**
 * @brief   Start finding Structures in g_structureFindArray.
 */
Structure *
Structure_FindFirst(PoolFindStruct *find,
		enum HouseType houseID, enum StructureType type)
{
	assert(houseID < HOUSE_MAX || houseID == HOUSE_INVALID);
	assert(type < STRUCTURE_MAX || type == STRUCTURE_INVALID);

	find->houseID = (houseID < HOUSE_MAX) ? houseID : HOUSE_INVALID;
	find->type    = (type < STRUCTURE_MAX) ? type : 0xFFFF;
	find->index   = 0xFFFF;

	return Structure_FindNext(find);
}

/**
 * @brief   Continue finding Structures in g_structureFindArray.
 */
Structure *
Structure_FindNext(PoolFindStruct *find)
{
	if (find->index >= g_structureFindCount + 3 && find->index != 0xFFFF)
		return NULL;

	/* First, go to the next index. */
	find->index++;

	assert(g_structureFindCount <= STRUCTURE_INDEX_MAX_SOFT);
	for (; find->index < g_structureFindCount + 3; find->index++) {
		Structure *s = NULL;

		if (find->index < g_structureFindCount) {
			s = g_structureFindArray[find->index];
		}
		else if (find->index == g_structureFindCount + 0) {
			s = Structure_Get_ByIndex(STRUCTURE_INDEX_WALL);
			assert(s->o.index == STRUCTURE_INDEX_WALL
			    && s->o.type == STRUCTURE_WALL);
		}
		else if (find->index == g_structureFindCount + 1) {
			s = Structure_Get_ByIndex(STRUCTURE_INDEX_SLAB_2x2);
			assert(s->o.index == STRUCTURE_INDEX_SLAB_2x2
			    && s->o.type == STRUCTURE_SLAB_2x2);
		}
		else if (find->index == g_structureFindCount + 2) {
			s = Structure_Get_ByIndex(STRUCTURE_INDEX_SLAB_1x1);
			assert(s->o.index == STRUCTURE_INDEX_SLAB_1x1
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
 */
void
Structure_Init(void)
{
	memset(g_structureArray, 0, sizeof(g_structureArray));
	memset(g_structureFindArray, 0, sizeof(g_structureFindArray));
	g_structureFindCount = 0;

	/* ENHANCEMENT -- Ensure the index is always valid. */
	for (unsigned int i = 0; i < STRUCTURE_INDEX_MAX_HARD; i++) {
		g_structureArray[i].o.index = i;
	}

	Structure_Allocate(0, STRUCTURE_SLAB_1x1);
	Structure_Allocate(0, STRUCTURE_SLAB_2x2);
	Structure_Allocate(0, STRUCTURE_WALL);
}

/**
 * @brief   Recount all Structures, rebuilding g_structureFindArray.
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

	g_structureFindCount = 0;

	for (unsigned int i = 0; i < STRUCTURE_INDEX_MAX_SOFT; i++) {
		Structure *s = Structure_Get_ByIndex(i);

		if (s->o.flags.s.used) {
			g_structureFindArray[g_structureFindCount] = s;
			g_structureFindCount++;
		}
	}
}

/**
 * @brief   Allocate a Structure.
 */
Structure *
Structure_Allocate(uint16 index, enum StructureType type)
{
	Structure *s = NULL;
	assert(type < STRUCTURE_MAX);

	switch (type) {
		case STRUCTURE_SLAB_1x1:
			index = STRUCTURE_INDEX_SLAB_1x1;
			s = Structure_Get_ByIndex(index);
			break;

		case STRUCTURE_SLAB_2x2:
			index = STRUCTURE_INDEX_SLAB_2x2;
			s = Structure_Get_ByIndex(index);
			break;

		case STRUCTURE_WALL:
			index = STRUCTURE_INDEX_WALL;
			s = Structure_Get_ByIndex(index);
			break;

		default:
			assert(index < STRUCTURE_INDEX_MAX_SOFT || index == STRUCTURE_INDEX_INVALID);

			if (index < STRUCTURE_INDEX_MAX_SOFT) {
				s = Structure_Get_ByIndex(index);
				if (s->o.flags.s.used)
					return NULL;
			}
			else {
				/* Find the first unused index. */
				for (index = 0; index < STRUCTURE_INDEX_MAX_SOFT; index++) {
					s = Structure_Get_ByIndex(index);
					if (!s->o.flags.s.used)
						break;
				}
				if (index == STRUCTURE_INDEX_MAX_SOFT)
					return NULL;
			}

			assert(g_structureFindCount < STRUCTURE_INDEX_MAX_SOFT);
			g_structureFindArray[g_structureFindCount] = s;
			g_structureFindCount++;
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
	assert(g_structureFindCount <= STRUCTURE_INDEX_MAX_SOFT);
	for (i = 0; i < g_structureFindCount; i++) {
		if (g_structureFindArray[i] == s)
			break;
	}

	/* We should always find an entry. */
	assert(i < g_structureFindCount);

	g_structureFindCount--;

	/* If needed, close the gap. */
	if (i < g_structureFindCount) {
		memmove(&g_structureFindArray[i], &g_structureFindArray[i + 1],
				(g_structureFindCount - i) * sizeof(g_structureFindArray[0]));
	}
}
