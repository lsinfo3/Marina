#include "flow_hashtable.h"

#include <string.h>
#include <stdatomic.h>

#include "hashes/MurmurHash3.h"

flow_hashtable_t flow_hashtable;

#if (FLOW_HASHTABLE_SIZE & (FLOW_HASHTABLE_SIZE - 1)) == 0
#define POSITIVE_MODULO(x) ((x) & (FLOW_HASHTABLE_SIZE - 1))
#else
//#warning "Using suboptimal operators, as MAX_FLOWS, NUM_PIPES, or FLOW_HASHTABLE_MULTIPLIER is not a power of 2"
int32_t modulo(int32_t x) {
	x %= FLOW_HASHTABLE_SIZE;
	if (x < 0) {
		x += FLOW_HASHTABLE_SIZE;
	}
	return x;
}
#define POSITIVE_MODULO(x) modulo(x)
#endif

#define LOCK(table) pthread_spin_lock(&(table)->lock)
#define UNLOCK(table) pthread_spin_unlock(&(table)->lock)
#define HASH2INDEX(hash) POSITIVE_MODULO(hash)
#define CAN_MOVE_ITEM_TO_CURRENT_EMPTY(item_desired_index, current_empty, item_index) \
    POSITIVE_MODULO((item_index) - (item_desired_index)) > \
    POSITIVE_MODULO((current_empty) - (item_desired_index))
#define MEMORY_BARRIER atomic_signal_fence(memory_order_acq_rel);

/*
 * Initialize the hashtable struct
 */
void flow_hashtable_init() {
	flow_hashtable.num_entries = 0;
	memset(flow_hashtable.entries, 0, sizeof(flow_hashtable_entry_t) * FLOW_HASHTABLE_SIZE);
	for (int i = 0; i < FEATURE_SIZE; i++) {
		flow_hashtable.feature_index_to_entry_map[i] = -1;
	}
	pthread_spin_init(&flow_hashtable.lock, 0);
}

static inline void flow_hashtable_hash_key(const five_tuple_t *key, uint32_t *hash) {
	MurmurHash3_x86_32(&key->h, sizeof(five_tuple_for_hash_t), MURMUR_SEED, hash);
}

/*
 * Add five tuple with flow id to hashtable, return 0 on success and 1 if hashtable is full
 */
int flow_hashtable_add(const five_tuple_t *key, const int32_t value) {
	if (flow_hashtable.num_entries == FLOW_HASHTABLE_SIZE) {
		return 1;
	}
	int ret = 0;
	uint32_t hash;
	flow_hashtable_hash_key(key, &hash);

	int32_t index = HASH2INDEX(hash);
//	LOCK(table);
	while (flow_hashtable.entries[index].occupied) {
		if (flow_hashtable.entries[index].hash == hash) {
			if (memcmp(&flow_hashtable.entries[index].key.h, &key->h, sizeof(five_tuple_for_hash_t)) == 0) {
				ret = 1;
				break;
			}
		}
		index = (index + 1) % (FLOW_HASHTABLE_SIZE);
	}
	if (ret == 0) {
		flow_hashtable.entries[index].key = *key;
		flow_hashtable.entries[index].value = value;
		flow_hashtable.entries[index].hash = hash;
		flow_hashtable.feature_index_to_entry_map[value] = index;
		flow_hashtable.num_entries++;
		MEMORY_BARRIER
		flow_hashtable.entries[index].occupied = true;
	}
//	UNLOCK(table);
	return ret;
}

/*
 * Helper function to find the index for a given key, searches for the first free index in
 * the hashtable. Returns 1 if key is not in the hashtable.
 * Hashtable MUST be locked!
 */
static inline int get_hashtable_index(const five_tuple_t *key, const uint32_t hash, int32_t *index) {
	*index = HASH2INDEX(hash);

	do {
		// If this entry is not occupied, the key is not in this table
		if (!flow_hashtable.entries[*index].occupied)
			return 1;
		// If occupied and hash matches
		if (flow_hashtable.entries[*index].hash == hash) {
			// check if the key also matches
			if (memcmp(&flow_hashtable.entries[*index].key.h, &key->h, sizeof(five_tuple_for_hash_t)) == 0) {
				return 0;
			}
		}
		// otherwise, search the next entry
		*index = (*index + 1) % (FLOW_HASHTABLE_SIZE);
	} while (1);
}

/*
 * Get the value for a given key. Return 0 if key is in hashtable, 1 otherwise.
 * Value can be NULL if it should be ignored.
 */
int flow_hashtable_get(const five_tuple_t *key, int32_t *value) {
	int32_t index;
	uint32_t hash;
	flow_hashtable_hash_key(key, &hash);
	int ret = get_hashtable_index(key, hash, &index);
	if (value != NULL && ret == 0) {
		*value = flow_hashtable.entries[index].value;
	}
	return ret;
}

static void remove_by_index(int32_t index) {
	flow_hashtable.entries[index].occupied = false;
	MEMORY_BARRIER
	flow_hashtable.entries[index].hash = 0;
	flow_hashtable.num_entries--;
	flow_hashtable.feature_index_to_entry_map[flow_hashtable.entries[index].value] = -1;

	// We found the and removed the value, now move all entries, which could not be placed on their
	// correct position on insertion, to their real place or at least nearer to it.
	// This ensures that for a given key k with hash h at index h+i, all entries from h to h+i are
	// occupied.
	int32_t empty_index = index;
	while (1) {
		// This loop will terminate, as we have at least one unoccupied entry in the hashtable
		index = (index + 1) % (FLOW_HASHTABLE_SIZE);
		// If we encounter an unoccupied entry, we can stop as there are no more entries after this,
		// which were misplaced due to our entry.
		if (!flow_hashtable.entries[index].occupied) {
			break;
		}
		// If we encounter an entry where the index does not match the hash,
		// move it to the previous free slot if that is a valid slot for this item
		if (index != HASH2INDEX(flow_hashtable.entries[index].hash)) {
			if (CAN_MOVE_ITEM_TO_CURRENT_EMPTY(HASH2INDEX(flow_hashtable.entries[index].hash), empty_index, index)) {
				flow_hashtable.entries[index].occupied = false;
				MEMORY_BARRIER
				flow_hashtable.entries[empty_index].hash = flow_hashtable.entries[index].hash;
				flow_hashtable.entries[empty_index].value = flow_hashtable.entries[index].value;
				flow_hashtable.entries[empty_index].key = flow_hashtable.entries[index].key;
				flow_hashtable.feature_index_to_entry_map[flow_hashtable.entries[index].value] = empty_index;
				MEMORY_BARRIER
				flow_hashtable.entries[empty_index].occupied = true;
				empty_index = index;
			}
		}
	}
}

/*
 * Remove a given key from the hash table. Return 0 if key is in hashtable, 1 otherwise.
 * Value can be NULL if it should be ignored.
 */
int flow_hashtable_remove(const five_tuple_t *key, int32_t *value) {
	int32_t index;
	int ret = 0;
	uint32_t hash;
	flow_hashtable_hash_key(key, &hash);
//	LOCK(table);

	int status = get_hashtable_index(key, hash, &index);
	if (status) {
		ret = status;
	}
	else {
		*value = flow_hashtable.entries[index].value;
		remove_by_index(index);
	}
//	UNLOCK(table);
	return ret;
}

int flow_hashtable_remove_by_value(const int32_t value, five_tuple_t *key) {
	int ret = 0;
//	LOCK(table);
	int32_t index = flow_hashtable.feature_index_to_entry_map[value];
	if (index == -1) {
		ret = 1;
	}
	else {
		if (key != NULL) {
			*key = flow_hashtable.entries[index].key;
		}
		remove_by_index(index);
	}
//	UNLOCK(table);
	return ret;
}
