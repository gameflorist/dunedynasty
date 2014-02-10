#ifndef BUILDQUEUE_H
#define BUILDQUEUE_H

#include "enumeration.h"
#include "types.h"

enum {
	OBJECTTYPE_MAX = 32
};
assert_compile((int)OBJECTTYPE_MAX >= STRUCTURE_MAX);
assert_compile((int)OBJECTTYPE_MAX >= UNIT_MAX);

typedef struct BuildQueueItem {
	struct BuildQueueItem *next;
	struct BuildQueueItem *prev;

	uint16 objectType;

	/* Credits spent.  Used for refunds, in particular starports. */
	int credits;
} BuildQueueItem;

typedef struct BuildQueue {
	BuildQueueItem *first;
	BuildQueueItem *last;

	int count[OBJECTTYPE_MAX];
} BuildQueue;

extern void BuildQueue_Init(BuildQueue *queue);
extern void BuildQueue_Free(BuildQueue *queue);

extern void BuildQueue_Add(BuildQueue *queue, uint16 objectType, int credits);
extern uint16 BuildQueue_RemoveHead(BuildQueue *queue);
extern bool BuildQueue_RemoveTail(BuildQueue *queue, uint16 objectType, int *credits);
extern bool BuildQueue_IsEmpty(const BuildQueue *queue);
extern int BuildQueue_Count(const BuildQueue *queue, uint16 objectType);
extern void BuildQueue_SetCount(BuildQueue *queue, uint16 objectType, int count);

#endif
