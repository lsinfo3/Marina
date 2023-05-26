#ifndef LOW_LEVEL_LIB_SESSION_HASHTABLE_H
#define LOW_LEVEL_LIB_SESSION_HASHTABLE_H

#include <stdbool.h>
#include <pthread.h>

#include "./structs.h"
#include "../config.h"

#define SESSION_HASHTABLE_SIZE (MAX_FLOWS * NUM_PIPES * FLOW_HASHTABLE_MULTIPLIER)


typedef struct {
	five_tuple_t key;
	int32_t value;
	uint32_t hash;
	bool occupied;
} session_hashtable_entry_t;

typedef struct {
	size_t num_entries;
	pthread_spinlock_t lock;
	session_hashtable_entry_t entries[SESSION_HASHTABLE_SIZE];
	int32_t feature_index_to_entry_map[FEATURE_SIZE];
	int32_t egress_feature_to_session_id[FEATURE_SIZE];
	int32_t flow_count[FEATURE_SIZE];
} session_hashtable_t;

extern session_hashtable_t session_hashtable;

void session_hashtable_init();

int session_hashtable_add_or_increase_flow_count(const five_tuple_t *key, const int32_t value);

int session_hashtable_get(const five_tuple_t *key, int32_t *value);

int session_hashtable_remove_or_decrease_flow_count(const five_tuple_t *key, int32_t *value);

int session_hashtable_remove_or_decrease_flow_count_by_value(const int32_t value, five_tuple_t *key);

void session_hashtable_copy(session_hashtable_t *dest);

#endif //LOW_LEVEL_LIB_SESSION_HASHTABLE_H
