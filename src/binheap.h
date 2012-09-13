#ifndef BINHEAP_H
#define BINHEAP_H

#include <inttypes.h>

typedef struct BinHeapElem {
	int64_t key;
} BinHeapElem;

typedef struct BinHeap {
	int num_elem;
	int max_elem;
	size_t elem_size;

	void *elem;
} BinHeap;

extern void BinHeap_Init(BinHeap *heap, size_t elem_size);
extern void BinHeap_Free(BinHeap *heap);
extern BinHeapElem *BinHeap_GetElem(BinHeap *heap, int i);

extern void *BinHeap_Push(BinHeap *heap, int64_t key);
extern void BinHeap_Pop(BinHeap *heap);
extern void *BinHeap_GetMin(BinHeap *heap);
extern void BinHeap_UpdateMin(BinHeap *heap);

#endif
