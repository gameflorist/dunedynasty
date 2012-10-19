/* binheap.c
 *
 * Binary min heap.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "binheap.h"

void
BinHeap_Init(BinHeap *heap, size_t elem_size)
{
	heap->num_elem = 1;

	if ((heap->elem != NULL) && (heap->elem_size == elem_size))
		return;

	heap->max_elem = 32;
	heap->elem_size = elem_size;

	free(heap->elem);
	heap->elem = malloc(heap->max_elem * heap->elem_size);
	assert(heap->elem != NULL);
}

void
BinHeap_Free(BinHeap *heap)
{
	heap->num_elem = 0;
	heap->max_elem = 0;
	heap->elem_size = 0;

	free(heap->elem);
	heap->elem = NULL;
}

BinHeapElem *
BinHeap_GetElem(BinHeap *heap, int i)
{
	return (BinHeapElem *)(((char *)heap->elem) + i * heap->elem_size);
}

void *
BinHeap_Push(BinHeap *heap, int64_t key)
{
	if (heap->num_elem >= heap->max_elem) {
		void *ptr = realloc(heap->elem, 2 * heap->max_elem * heap->elem_size);

		if (ptr == NULL)
			return NULL;

		heap->max_elem *= 2;
		heap->elem = ptr;
	}

	int idx = heap->num_elem;
	BinHeapElem *e = BinHeap_GetElem(heap, idx);

	/* Sift up. */
	while (idx > 1) {
		int parent = idx / 2;
		BinHeapElem *p = BinHeap_GetElem(heap, parent);

		if (p->key <= key)
			break;

		memcpy(e, p, heap->elem_size);
		idx = parent;
		e = p;
	}

	e->key = key;
	heap->num_elem++;
	return e;
}

void
BinHeap_Pop(BinHeap *heap)
{
	int idx = 1;
	BinHeapElem *e = BinHeap_GetElem(heap, idx);
	BinHeapElem *last = BinHeap_GetElem(heap, heap->num_elem - 1);

	while (2 * idx < heap->num_elem) {
		int child = 2 * idx;
		int right = 2 * idx + 1;
		BinHeapElem *c = BinHeap_GetElem(heap, child);

		if (right < heap->num_elem) {
			BinHeapElem *r = BinHeap_GetElem(heap, right);

			if (r->key < c->key) {
				child = right;
				c = r;
			}
		}

		if (last->key <= c->key)
			break;

		memcpy(e, c, heap->elem_size);
		idx = child;
		e = c;
	}

	if (e != last)
		memcpy(e, last, heap->elem_size);

	heap->num_elem--;
}

void *
BinHeap_GetMin(BinHeap *heap)
{
	if (heap->num_elem <= 1)
		return NULL;

	return BinHeap_GetElem(heap, 1);
}

void
BinHeap_UpdateMin(BinHeap *heap)
{
	int idx = 1;
	BinHeapElem *tmp = BinHeap_GetElem(heap, 0);
	BinHeapElem *e = BinHeap_GetElem(heap, 1);

	/* Use heap->elem[0] as temporary storage. */
	memcpy(tmp, e, heap->elem_size);

	while (2 * idx < heap->num_elem) {
		int child = 2 * idx;
		int right = 2 * idx + 1;
		BinHeapElem *c = BinHeap_GetElem(heap, child);

		if (right < heap->num_elem) {
			BinHeapElem *r = BinHeap_GetElem(heap, right);

			if (r->key < c->key) {
				child = right;
				c = r;
			}
		}

		if (tmp->key <= c->key) {
			break;
		}
		else {
			memcpy(e, c, heap->elem_size);
			idx = child;
			e = c;
		}
	}

	memcpy(e, tmp, heap->elem_size);
}
