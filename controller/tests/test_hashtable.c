#include "test_macros.h"

#include <sys/time.h>
#include <stdlib.h>

#include "../datastructures/flow_hashtable.h"

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

int main(void) {
	PREPARE_TIMING
	printf("start test\n");
	flow_hashtable_init();
	PRINT_TIME_DELTA("init")
	ASSERT_EQUAL((int) flow_hashtable.num_entries, 0)
	five_tuple_t key = {
			.h.src_addr.ipv4 = 0
	};
	int32_t value = 11;

	flow_hashtable_add(&key, value);

	ASSERT_EQUAL((int) flow_hashtable.num_entries, 1)

	int32_t result;
	int status = flow_hashtable_get(&key, &result);
	ASSERT_EQUAL(status, 0)
	ASSERT_EQUAL(result, value)

	status = flow_hashtable_remove(&key, &result);
	ASSERT_EQUAL(status, 0)
	ASSERT_EQUAL(result, value)

	status = flow_hashtable_remove(&key, &result);
	ASSERT_EQUAL(status, 1)

	PRINT_TIME_DELTA("single")

	for (int i = 0; i < 10; i++) {
		key.h.dst_port = i;
		value = i;
		flow_hashtable_add(&key, value);
	}
	ASSERT_EQUAL((int) flow_hashtable.num_entries, 10)

	for (int i = 0; i < 10; i++) {
		key.h.dst_port = i;
		ASSERT_EQUAL(flow_hashtable_get(&key, &value), 0)
		ASSERT_EQUAL(value, i);
	}
	ASSERT_EQUAL((int) flow_hashtable.num_entries, 10)

	for (int i = 9; i >=0; i--) {
		key.h.dst_port = i;
		ASSERT_EQUAL(flow_hashtable_get(&key, &value), 0)
		ASSERT_EQUAL(value, i);
	}

	for (int i = 0; i < 10; i++) {
		key.h.dst_port = i;
		ASSERT_EQUAL(flow_hashtable_remove(&key, &value), 0)
		ASSERT_EQUAL(value, i);
	}
	ASSERT_EQUAL((int) flow_hashtable.num_entries, 0)

	PRINT_TIME_DELTA("tens")

	for (int k = 0; k < 20; k++) {
		srand48(k);

		int len = FLOW_HASHTABLE_SIZE / FLOW_HASHTABLE_MULTIPLIER;
		int32_t array[len];
		for (int32_t i = 0; i < len; i++) {
			array[i] = i;
		}
		shuffle(array, len);

		PRINT_TIME_DELTA("shuffle")

		for (int i = 0; i < len / 2; i++) {
			key.h.dst_addr.ipv4 = array[i];
			ASSERT_EQUAL(flow_hashtable_add(&key, i), 0)
		}

		PRINT_TIME_DELTA("insert half")

		shuffle(array, len / 4);
		for (int i = 0; i < len / 4; i++) {
			key.h.dst_addr.ipv4 = array[i];
			ASSERT_EQUAL(flow_hashtable_remove(&key, &value), 0)
		}

		PRINT_TIME_DELTA("remove quarter")

		for (int i = len / 2; i < len; i++) {
			key.h.dst_addr.ipv4 = array[i];
			ASSERT_EQUAL(flow_hashtable_add(&key, i), 0)
		}

		PRINT_TIME_DELTA("insert half")

		shuffle(array + len / 4, len / 4);
		for (int i = len / 4; i < len / 2; i++) {
			key.h.dst_addr.ipv4 = array[i];
			ASSERT_EQUAL(flow_hashtable_remove(&key, &value), 0)
		}

		PRINT_TIME_DELTA("remove quarter")

		for (int i = 0; i < len / 2; i++) {
			key.h.dst_addr.ipv4 = array[i];
			ASSERT_EQUAL(flow_hashtable_add(&key, i), 0)
		}

		PRINT_TIME_DELTA("insert half")

		shuffle(array, len);
		for (int i = 0; i < len; i++) {
			key.h.dst_addr.ipv4 = array[i];
			ASSERT_EQUAL(flow_hashtable_remove(&key, &value), 0)
		}

		PRINT_TIME_DELTA("remove all")
	}

	printf("success\n");
}
