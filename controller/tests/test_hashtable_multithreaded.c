#include "test_macros.h"

#include <stdlib.h>
#include <pthread.h>

#include "../datastructures/flow_hashtable.h"

#define TESTARRAY_LEN (FLOW_HASHTABLE_SIZE / FLOW_HASHTABLE_MULTIPLIER)

#define RUNS 20

void shuffle(int *array, size_t n) {
	if (n > 1) {
		size_t i;
		for (i = n - 1; i > 0; i--) {
			size_t j = (unsigned int) (drand48() * (i + 1));
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

void *reader(void *args) {
	five_tuple_t key = {};
	int32_t value;
	int32_t *array = (int32_t *) args;

	printf("Starting reader\n");
	for (int i = 0; i < RUNS; i++) {
		printf("Reader: %d\n", i);
		for (int j = 0; j < TESTARRAY_LEN; j++) {
			key.h.dst_addr.ipv4 = array[j];
			if (flow_hashtable_remove(&key, &value) == 0) {
				if (value != array[j]) {
					printf("Expected %d, got %d", array[j], value);
				}
			}
		}
	}
	printf("Stopping reader\n");


	return NULL;
}

void *writer(void *args) {
	five_tuple_t key = {};
	int32_t value;
	int32_t *array = (int32_t *) args;

	printf("Starting writer\n");
	for (int i = 0; i < RUNS; i++) {
		printf("Writer: %d\n", i);
		for (int j = 0; j < TESTARRAY_LEN; j++) {
			key.h.dst_addr.ipv4 = array[j];
			value = array[j];
			flow_hashtable_add(&key, value);
		}
	}
	printf("Stopping writer\n");
	return NULL;
}

int main(void) {
	flow_hashtable_init();

	static int32_t array[TESTARRAY_LEN];
	for (int32_t i = 0; i < TESTARRAY_LEN; i++) {
		array[i] = i;
	}
	shuffle(array, TESTARRAY_LEN);

	PREPARE_TIMING

	pthread_t reader_thread, writer_thread;
	pthread_create(&reader_thread, NULL, reader, array);
	pthread_create(&writer_thread, NULL, writer, array);

	pthread_join(reader_thread, NULL);
	pthread_join(writer_thread, NULL);

	PRINT_TIME_DELTA("Took")

	printf("success\n");
}
