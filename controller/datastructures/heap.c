#include <stddef.h>

#include "heap.h"

#define HEAP_KEY(i) (heap->data[(i)])
#define HEAP_LEFT(i) (2*(i) + 1)
#define HEAP_RIGHT(i) (2*(i) + 2)
#define HEAP_PARENT(i) (((i) - 1) / 2)
#define HEAP_SWAP(x, y) {          \
    heap_item_t temp = HEAP_KEY(x); \
    HEAP_KEY(x) = HEAP_KEY(y);    \
    HEAP_KEY(y) = temp;          \
    }

static void heapify(heapqueue_t *heap, size_t a) {
	size_t i = a;
	do {
		size_t min = i;
		if (HEAP_LEFT(i) < heap->curSize && HEAP_KEY(HEAP_LEFT(i)) < HEAP_KEY(min))
			min = HEAP_LEFT(i);
		if (HEAP_RIGHT(i) < heap->curSize && HEAP_KEY(HEAP_RIGHT(i)) < HEAP_KEY(min))
			min = HEAP_RIGHT(i);
		if (min == i)
			break;
		HEAP_SWAP(i, min)
		i = min;
	} while (1);
}

void heap_build(heapqueue_t *heap, size_t size) {
	heap->curSize = size;
	if (size < 2)
		return;
	for (size_t i = heap->curSize / 2 - 1; i < heap->curSize && i >= 0; i--) {
		heapify(heap, i);
	}
}

static inline void heap_decrease(heapqueue_t *heap, size_t index, heap_item_t key) {
	if (HEAP_KEY(index) < key)
		return;

	HEAP_KEY(index) = key;
	while (index > 0 && HEAP_KEY(index) < HEAP_KEY(HEAP_PARENT(index))) {
		HEAP_SWAP(index, HEAP_PARENT(index))
		index = HEAP_PARENT(index);
	}
}

void heap_push(heapqueue_t *heap, heap_item_t item) {
	size_t i = heap->curSize++;
	HEAP_KEY(i) = item;

	heap_decrease(heap, i, item);
}

heap_item_t heap_peek(heapqueue_t *heap) {
	return heap->data[0];
}


heap_item_t heap_pop(heapqueue_t *heap) {
	if (heap->curSize == 0)
		return 0;

	heap_item_t result = HEAP_KEY(0);
	size_t last = --heap->curSize;
	if (last == 0)
		return result;

	HEAP_SWAP(0, last)
	heapify(heap, 0);
	return result;
}