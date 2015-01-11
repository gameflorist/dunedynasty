/** @file src/saveload/structure.c Load/save routines for Structure. */

#include <string.h>

#include "saveload.h"
#include "../house.h"
#include "../opendune.h"
#include "../pool/pool.h"
#include "../pool/pool_house.h"
#include "../pool/pool_structure.h"
#include "../pool/pool_unit.h"
#include "../structure.h"
#include "../unit.h"

static uint32 SaveLoad_Structure_BuildQueue(void *object, uint32 value, bool loading);

static const SaveLoadDesc s_saveStructure[] = {
	SLD_SLD   (Structure,              o, g_saveObject),
	SLD_ENTRY (Structure, SLDT_UINT16, creatorHouseID),
	SLD_ENTRY (Structure, SLDT_UINT16, rotationSpriteDiff),
	SLD_EMPTY (           SLDT_UINT8),
	SLD_ENTRY (Structure, SLDT_UINT16, objectType),
	SLD_ENTRY (Structure, SLDT_UINT8,  upgradeLevel),
	SLD_ENTRY (Structure, SLDT_UINT8,  upgradeTimeLeft),
	SLD_ENTRY (Structure, SLDT_UINT16, countDown),
	SLD_ENTRY (Structure, SLDT_UINT16, buildCostRemainder),
	SLD_ENTRY (Structure,  SLDT_INT16, state),
	SLD_ENTRY (Structure, SLDT_UINT16, hitpointsMax),
	SLD_END
};

static const SaveLoadDesc s_saveStructure2[] = {
	SLD_ENTRY (Structure, SLDT_UINT16, o.index),
	SLD_ENTRY2(Structure, SLDT_UINT8,  squadID, SLDT_UINT32),
	SLD_ENTRY (Structure, SLDT_UINT16, rallyPoint),
	SLD_ENTRY (Structure, SLDT_INT32,  factoryOffsetY),
	SLD_CUSTOM(Structure, queue, SaveLoad_Structure_BuildQueue),
	SLD_END
};
assert_compile(sizeof(enum SquadID) == sizeof(uint32));

static const SaveLoadDesc s_saveBuildQueue[] = {
	SLD_ENTRY (BuildQueueItem, SLDT_UINT16, objectType),
	SLD_ENTRY (BuildQueueItem, SLDT_INT32,  credits),
	SLD_END
};

/**
 * Load all Structures from a file.
 * @param fp The file to load from.
 * @param length The length of the data chunk.
 * @return True if and only if all bytes were read successful.
 */
bool Structure_Load(FILE *fp, uint32 length)
{
	while (length > 0) {
		Structure *s;
		Structure sl;

		memset(&sl, 0, sizeof(sl));

		/* Read the next Structure from disk */
		if (!SaveLoad_Load(s_saveStructure, fp, &sl)) return false;

		length -= SaveLoad_GetLength(s_saveStructure);

		sl.o.script.scriptInfo = g_scriptStructure;
		sl.o.script.script = g_scriptStructure->start + (size_t)sl.o.script.script;
		if (sl.upgradeTimeLeft == 0) sl.upgradeTimeLeft = Structure_IsUpgradable(&sl) ? 100 : 0;

		/* Get the Structure from the pool */
		s = Structure_Get_ByIndex(sl.o.index);
		if (s == NULL) return false;

		/* Copy over the data */
		*s = sl;

		/* Extra data. */
		s->squadID = SQUADID_INVALID;
		BuildQueue_Init(&s->queue);
		s->rallyPoint = 0xFFFF;
		s->factoryOffsetY = 0;
	}
	if (length != 0) return false;

	Structure_Recount();

	return true;
}

/**
 * Save all Structures to a file. It converts pointers to indices where needed.
 * @param fp The file to save to.
 * @return True if and only if all bytes were written successful.
 */
bool Structure_Save(FILE *fp)
{
	PoolFindStruct find;

	for (const Structure *s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
			s != NULL;
			s = Structure_FindNext(&find)) {
		Structure ss = *s;

		if (!SaveLoad_Save(s_saveStructure, fp, &ss)) return false;
	}

	return true;
}

/*--------------------------------------------------------------*/

bool
Structure_Load2(FILE *fp, uint32 length)
{
	while (length > 0) {
		Structure sl;
		if (!SaveLoad_Load(s_saveStructure2, fp, &sl))
			return false;

		SaveLoad_CustomCallbackData data;
		data.fp = NULL;
		data.object = &sl;

		length -= SaveLoad_GetLength(s_saveStructure2);
		length -= SaveLoad_Structure_BuildQueue(&data, 0, true);

		Structure *s = Structure_Get_ByIndex(sl.o.index);
		if (s == NULL)
			return false;

		/* Extra data. */
		s->squadID = sl.squadID;
		s->rallyPoint = sl.rallyPoint;
		s->factoryOffsetY = sl.factoryOffsetY;

		/* Old saved game files had separate starport queues.
		 * Append these to the House starport queue.
		 */
		if (s->o.type == STRUCTURE_STARPORT) {
			House *h = House_Get_ByIndex(s->o.houseID);

			if (BuildQueue_IsEmpty(&h->starportQueue)) {
				h->starportQueue = sl.queue;
			} else {
				sl.queue.first->prev = h->starportQueue.last;
				h->starportQueue.last->next = sl.queue.first;
				h->starportQueue.last = sl.queue.last;
			}

			for (enum UnitType u = UNIT_CARRYALL; u < UNIT_MAX; u++) {
				h->starportCount[u] = BuildQueue_Count(&h->starportQueue, u);
			}
		} else {
			s->queue = sl.queue;
		}
	}

	if (length != 0)
		return false;

	return true;
}

bool
Structure_Save2(FILE *fp)
{
	PoolFindStruct find;

	for (Structure *s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
			s != NULL;
			s = Structure_FindNext(&find)) {
		if (!SaveLoad_Save(s_saveStructure2, fp, s))
			return false;
	}

	return true;
}

static uint32
SaveLoad_Structure_BuildQueue(void *object, uint32 value, bool loading)
{
	SaveLoad_CustomCallbackData *data = object;
	FILE *fp = data->fp;
	Structure *s = data->object;
	uint32 size = 0;
	uint32 elem_size = SaveLoad_GetLength(s_saveBuildQueue);
	VARIABLE_NOT_USED(value);

	/* If fp == NULL, then it is a size query. */
	if (fp == NULL) {
		uint32 count = BuildQueue_Count(&s->queue, 0xFFFF);

		size = sizeof(count) + count * elem_size;
	} else if (loading) {
		uint32 count;

		if (fread(&count, sizeof(uint32), 1, fp) != 1)
			return 0;

		size += sizeof(count);

		/* Saved games from older versions of Dune Dynasty do not have
		 * purchased units created.
		 */
		const Structure *s2 = Structure_Get_ByIndex(s->o.index);
		House *h = House_Get_ByIndex(s2->o.houseID);
		const bool create_starport_units
			= (s2->o.type == STRUCTURE_STARPORT)
			&& (h->starportLinkedID == UNIT_INDEX_INVALID);

		BuildQueue_Init(&s->queue);
		while (count > 0) {
			BuildQueueItem item;
			if (!SaveLoad_Load(s_saveBuildQueue, fp, &item))
				return 0;

			size += elem_size;

			bool insert_item = true;

			if (create_starport_units) {
				tile32 tile;
				Unit *u;

				g_validateStrictIfZero++;
				tile.x = 0xFFFF;
				tile.y = 0xFFFF;
				u = Unit_Create(UNIT_INDEX_INVALID, item.objectType, h->index, tile, 0);
				g_validateStrictIfZero--;

				if (u == NULL) {
					h->credits += item.credits;
					insert_item = false;
				} else {
					u->o.linkedID = h->starportLinkedID & 0xFF;
					h->starportLinkedID = u->o.index;
					h->starportTimeLeft = 0;
					insert_item = true;
				}
			}

			if (insert_item) {
				BuildQueue_Add(&s->queue, item.objectType, item.credits);
			}

			count--;
		}
	} else {
		/* Save starport queue into first starport for backwards
		 * compatability.
		 */
		BuildQueue *queue = &s->queue;
		if (s->o.type == STRUCTURE_STARPORT) {
			PoolFindStruct find;

			if (s == Structure_FindFirst(&find, s->o.houseID, STRUCTURE_STARPORT)) {
				House *h = House_Get_ByIndex(s->o.houseID);
				queue = &h->starportQueue;
			}
		}

		uint32 count = BuildQueue_Count(queue, 0xFFFF);

		if (fwrite(&count, sizeof(uint32), 1, fp) != 1)
			return 0;

		size += sizeof(count);

		BuildQueueItem *item = queue->first;
		while (item != NULL) {
			if (!SaveLoad_Save(s_saveBuildQueue, fp, item))
				return 0;

			size += elem_size;
			item = item->next;
		}
	}

	return size;
}
