/* buildqueue.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buildqueue.h"

void
BuildQueue_Init(BuildQueue *queue)
{
	queue->first = NULL;
	queue->last = NULL;

	memset(queue->count, 0, sizeof(queue->count));
}

void
BuildQueue_Free(BuildQueue *queue)
{
	BuildQueueItem *e = queue->first;

	while (e != NULL) {
		BuildQueueItem *nx = e->next;

		free(e);
		e = nx;
	}

	BuildQueue_Init(queue);
}

static BuildQueueItem *
BuildQueue_AllocItem(uint16 objectType, int credits)
{
	BuildQueueItem *e = malloc(sizeof(*e));
	assert(e);

	e->next = NULL;
	e->prev = NULL;
	e->objectType = objectType;
	e->credits = credits;

	return e;
}

void
BuildQueue_Add(BuildQueue *queue, uint16 objectType, int credits)
{
	BuildQueueItem *e = BuildQueue_AllocItem(objectType, credits);
	assert(objectType < OBJECTTYPE_MAX);

	if (queue->count[objectType] >= 99)
		return;
	
	if (queue->first == NULL)
		queue->first = e;

	if (queue->last != NULL) {
		queue->last->next = e;
		e->prev = queue->last;
	}

	queue->last = e;
	queue->count[objectType]++;
}

uint16
BuildQueue_RemoveHead(BuildQueue *queue)
{
	BuildQueueItem *e = queue->first;
	uint16 ret = 0xFFFF;

	if (e != NULL) {
		ret = e->objectType;

		if (e->next != NULL)
			e->next->prev = NULL;

		queue->first = e->next;

		if (queue->last == e)
			queue->last = NULL;

		queue->count[ret]--;

		assert(e->prev == NULL);
		free(e);
	}

	return ret;
}

bool
BuildQueue_RemoveTail(BuildQueue *queue, uint16 objectType, int *credits)
{
	BuildQueueItem *e = queue->last;

	while (e != NULL) {
		if (e->objectType == objectType) {
			if (e->next != NULL)
				e->next->prev = e->prev;

			if (e->prev != NULL)
				e->prev->next = e->next;

			if (queue->first == e)
				queue->first = e->next;

			if (queue->last == e)
				queue->last = e->prev;

			if (credits != NULL)
				*credits = e->credits;

			queue->count[objectType]--;

			free(e);
			return true;
		} else {
			e = e->prev;
		}
	}

	if (credits != NULL)
		*credits = 0;

	return false;
}

bool
BuildQueue_IsEmpty(const BuildQueue *queue)
{
	return (queue->first == NULL);
}

int
BuildQueue_Count(const BuildQueue *queue, uint16 objectType)
{
	if (objectType < OBJECTTYPE_MAX) {
		return queue->count[objectType];
	} else {
		int count = 0;
		assert(objectType == 0xFFFF);

		for (objectType = 0; objectType < OBJECTTYPE_MAX; objectType++) {
			count += queue->count[objectType];
		}

		return count;
	}
}

void
BuildQueue_SetCount(BuildQueue *queue, uint16 objectType, int count)
{
	assert(objectType < OBJECTTYPE_MAX);
	queue->count[objectType] = count;
}
