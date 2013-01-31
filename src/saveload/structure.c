/** @file src/saveload/structure.c Load/save routines for Structure. */

#include <string.h>

#include "saveload.h"
#include "../pool/pool.h"
#include "../pool/structure.h"
#include "../structure.h"

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

	find.houseID = HOUSE_INVALID;
	find.type    = 0xFFFF;
	find.index   = 0xFFFF;

	while (true) {
		Structure *s;
		Structure ss;

		s = Structure_Find(&find);
		if (s == NULL) break;
		ss = *s;

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
		s->queue = sl.queue;
	}

	if (length != 0)
		return false;

	return true;
}

bool
Structure_Save2(FILE *fp)
{
	PoolFindStruct find;

	find.houseID = HOUSE_INVALID;
	find.type    = 0xFFFF;
	find.index   = 0xFFFF;

	while (true) {
		Structure *s = Structure_Find(&find);
		if (s == NULL)
			break;

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
	}
	else if (loading) {
		uint32 count;

		if (fread(&count, sizeof(uint32), 1, fp) != 1)
			return 0;

		size += sizeof(count);

		BuildQueue_Init(&s->queue);
		while (count > 0) {
			BuildQueueItem item;
			if (!SaveLoad_Load(s_saveBuildQueue, fp, &item))
				return 0;

			size += elem_size;
			BuildQueue_Add(&s->queue, item.objectType, item.credits);
			count--;
		}
	}
	else {
		uint32 count = BuildQueue_Count(&s->queue, 0xFFFF);

		if (fwrite(&count, sizeof(uint32), 1, fp) != 1)
			return 0;

		size += sizeof(count);

		BuildQueueItem *item = s->queue.first;
		while (item != NULL) {
			if (!SaveLoad_Save(s_saveBuildQueue, fp, item))
				return 0;

			size += elem_size;
			item = item->next;
		}
	}

	return size;
}
