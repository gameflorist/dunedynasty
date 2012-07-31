#ifndef BUILDQUEUE_H
#define BUILDQUEUE_H

#include "types.h"

typedef struct BuildQueueItem {
	struct BuildQueueItem *next;
	struct BuildQueueItem *prev;

	uint16 objectType;
} BuildQueueItem;

typedef struct BuildQueue {
	BuildQueueItem *first;
	BuildQueueItem *last;
} BuildQueue;

extern void BuildQueue_Init(BuildQueue *queue);
extern void BuildQueue_Free(BuildQueue *queue);

extern void BuildQueue_Add(BuildQueue *queue, uint16 objectType);
extern uint16 BuildQueue_RemoveHead(BuildQueue *queue);
extern void BuildQueue_RemoveTail(BuildQueue *queue, uint16 objectType);
extern int BuildQueue_Count(const BuildQueue *queue, uint16 objectType);

#endif
