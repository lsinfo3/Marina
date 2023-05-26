#ifndef LOW_LEVEL_LIB_HEAP_H
#define LOW_LEVEL_LIB_HEAP_H

#include <stdint.h>
#include <stddef.h>
#include "../config.h"


#define HEAPSIZE (MAX_FLOWS - 1)

typedef int32_t heap_item_t;

typedef struct {
	size_t curSize;
	heap_item_t data[HEAPSIZE];
} heapqueue_t;

void heap_build(heapqueue_t *heap, size_t size);

void heap_push(heapqueue_t *heap, heap_item_t item);

heap_item_t heap_peek(heapqueue_t *heap);

heap_item_t heap_pop(heapqueue_t *heap);

#endif //LOW_LEVEL_LIB_HEAP_H
