#ifndef LOW_LEVEL_LIB_FLOW_HASHTABLE_H
#define LOW_LEVEL_LIB_FLOW_HASHTABLE_H

#include <stdbool.h>
#include <pthread.h>

#include "./structs.h"
#include "../config.h"

#define FLOW_HASHTABLE_SIZE (MAX_FLOWS * NUM_PIPES * FLOW_HASHTABLE_MULTIPLIER)


typedef struct {
	five_tuple_t key;
	int32_t value;
	uint32_t hash;
	bool occupied;
} flow_hashtable_entry_t;

typedef struct {
	size_t num_entries;
	pthread_spinlock_t lock;
	flow_hashtable_entry_t entries[FLOW_HASHTABLE_SIZE];
	int32_t feature_index_to_entry_map[FEATURE_SIZE];
} flow_hashtable_t;

extern flow_hashtable_t flow_hashtable;

void flow_hashtable_init();

int flow_hashtable_add(const five_tuple_t *key, int32_t value);

int flow_hashtable_get(const five_tuple_t *key, int32_t *value);

int flow_hashtable_remove(const five_tuple_t *key, int32_t *value);

int flow_hashtable_remove_by_value(int32_t value, five_tuple_t *key);

#endif //LOW_LEVEL_LIB_FLOW_HASHTABLE_H
