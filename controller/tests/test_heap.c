#include "../datastructures/heap.h"

#include "test_macros.h"

int main(void) {
	printf("Testing heap\n");
	heapqueue_t heap;
	for (int i = 0; i < 10; i++) {
		heap.data[i] = i + 1;
	}
	heap_build(&heap, 10);

	for (int i = 0; i < 10; i++) {
		int res = heap_pop(&heap);
		ASSERT_EQUAL(i + 1, res)
	}

	for (int i = 0; i < 10; i++) {
		heap_push(&heap, i + 1);
	}
	for (int i = 0; i < 10; i++) {
		int res = heap_pop(&heap);
		ASSERT_EQUAL(i + 1, res)
	}
	printf("success\n");
	return 0;
}