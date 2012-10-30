/* buildqueue.c */

#include <assert.h>
#include <stdlib.h>

#include "buildqueue.h"

void
BuildQueue_Init(BuildQueue *queue)
{
	queue->first = NULL;
	queue->last = NULL;
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

	queue->first = NULL;
	queue->last = NULL;
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
	
	if (queue->first == NULL)
		queue->first = e;

	if (queue->last != NULL) {
		queue->last->next = e;
		e->prev = queue->last;
	}

	queue->last = e;
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

			free(e);
			return true;
		}
		else {
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
	BuildQueueItem *e = queue->first;
	int count = 0;

	while (e != NULL) {
		if ((e->objectType == objectType) || (objectType == 0xFFFF))
			count++;

		e = e->next;
	}

	return count;
}
