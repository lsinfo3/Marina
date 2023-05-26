#include <stdint.h>
#include <pthread.h>
#include "test_macros.h"

#include "../datastructures/mk_ringbuffer.h"


#define RINGBUFFER_SIZE 1024

typedef struct {
	int key;
} buf_item_t;

MK_RINGBUFFER_HEADER(test, buf_item_t, RINGBUFFER_SIZE)

GLOBAL_STAT_IMPL(teststat)

MK_RINGBUFFER_IMPL(test, buf_item_t, RINGBUFFER_SIZE, teststat)


void *reader(void *arg) {
	bool *test = (bool*) arg;
	for (int i = 0; i < 1 << 20; i++) {
		buf_item_t item;
		while (test_ringbuffer_empty());
		ASSERT_EQUAL(test_ringbuffer_get(&item), 0);
		if (*test)
			ASSERT_EQUAL(item.key, i)
	}
	return NULL;
}

void *writer(void *arg) {
	for (int i = 0; i < 1 << 20; i++) {
		buf_item_t item;
		item.key = i;
		test_ringbuffer_put(&item);
	}
	return NULL;
}

void *writer_ensure(void *arg) {
	for (int i = 0; i < 1 << 20; i++) {
		buf_item_t item;
		item.key = i;
		test_ringbuffer_ensure_put(&item);
	}
	return NULL;
}

int main(void) {
	printf("Testing ringbuffer\n");

	test_ringbuffer_init();

	for (int i = 0; i < 10; i++) {
		ASSERT_EQUAL(test_ringbuffer_length(), i);
		buf_item_t item = {
				.key = i
		};
		test_ringbuffer_put(&item);
	}

	ASSERT_EQUAL(test_ringbuffer_length(), 10);
	printf("Reading\n");

	for (int i = 0; i < 10; i++) {
		buf_item_t item;
		test_ringbuffer_get(&item);
		ASSERT_EQUAL(i, item.key)
	}

	printf("Threaded test\n");

	pthread_t reader_thread, writer_thread;
	bool f = false;
	pthread_create(&reader_thread, NULL, reader, (void*) &f);
	pthread_create(&writer_thread, NULL, writer, NULL);

	pthread_join(reader_thread, NULL);
	pthread_join(writer_thread, NULL);

	bool t = true;
	pthread_create(&reader_thread, NULL, reader, (void*) &t);
	pthread_create(&writer_thread, NULL, writer_ensure, NULL);

	pthread_join(reader_thread, NULL);
	pthread_join(writer_thread, NULL);

	printf("success\n");
	return 0;
}