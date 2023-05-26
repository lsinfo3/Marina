#include "session_hashtable.h"

#include <string.h>
#include <stdatomic.h>

#include "hashes/MurmurHash3.h"
#include "../runtime_config/runtime_config.h"

session_hashtable_t session_hashtable;

#if (SESSION_HASHTABLE_SIZE & (SESSION_HASHTABLE_SIZE - 1)) == 0
#define POSITIVE_MODULO(x) ((x) & (SESSION_HASHTABLE_SIZE - 1))
#else
//#warning "Using suboptimal operators, as MAX_SESSIONS, NUM_PIPES, or SESSION_HASHTABLE_MULTIPLIER is not a power of 2"
int32_t modulo(int32_t x) {
	x %= SESSION_HASHTABLE_SIZE;
	if (x < 0) {
		x += SESSION_HASHTABLE_SIZE;
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
void session_hashtable_init() {
	session_hashtable.num_entries = 0;
	memset(session_hashtable.entries, 0, sizeof(session_hashtable_entry_t) * SESSION_HASHTABLE_SIZE);
	for (int i = 0; i < FEATURE_SIZE; i++) {
		session_hashtable.feature_index_to_entry_map[i] = -1;
		session_hashtable.egress_feature_to_session_id[i] = -1;
	}
	pthread_spin_init(&session_hashtable.lock, 0);
}

static inline void session_hashtable_hash_key(const five_tuple_t *key, uint32_t *hash) {
	if (use_sessions()){
		MurmurHash3_x86_32(&key->sh, sizeof(five_tuple_for_session_hash_t), MURMUR_SEED, hash);
	}
	else {
		MurmurHash3_x86_32(&key->h, sizeof(five_tuple_for_hash_t), MURMUR_SEED, hash);
	}
}

static inline int compare_five_tuples(const five_tuple_t *key1, const five_tuple_t *key2) {
	if (use_sessions()) {
		return memcmp(&key1->sh, &key2->sh, sizeof(five_tuple_for_session_hash_t));
	}
	else {
		return memcmp(&key1->h, &key2->h, sizeof(five_tuple_for_hash_t));
	}
}

/*
 * Add five tuple with session id to hashtable, return 0 on success and 1 if hashtable is full
 */
int session_hashtable_add_or_increase_flow_count(const five_tuple_t *key, const int32_t value) {
	if (session_hashtable.num_entries == SESSION_HASHTABLE_SIZE) {
		return 1;
	}
	int ret = 0;
	uint32_t hash;
	session_hashtable_hash_key(key, &hash);

	int32_t index = HASH2INDEX(hash);
//	LOCK(table);
	while (session_hashtable.entries[index].occupied) {
		if (session_hashtable.entries[index].hash == hash) {
			if (compare_five_tuples(&session_hashtable.entries[index].key, key) == 0) {
				if (use_sessions()) {
					// the same session already exists, this means we have a new flow for this session
					session_hashtable.flow_count[value]++;
				}
				ret = 1;
				break;
			}
		}
		index = (index + 1) % (SESSION_HASHTABLE_SIZE);
	}
	if (ret == 0) {
		session_hashtable.entries[index].key = *key;
		session_hashtable.entries[index].value = value;
		session_hashtable.entries[index].hash = hash;
		session_hashtable.feature_index_to_entry_map[value] = index;
		session_hashtable.egress_feature_to_session_id[value + key->session_id_egress_offset] = value;
		session_hashtable.num_entries++;
		MEMORY_BARRIER
		session_hashtable.entries[index].occupied = true;
	}
//	UNLOCK(table);
	return ret;
}

/*
 * Helper function to find the index for a given key, searches for the first free index in
 * the hashtable. Returns 1 if key is not in the hashtable.
 * Hashtable MUST be locked!
 */
static inline int session_get_hashtable_index(const five_tuple_t *key, const uint32_t hash, int32_t *index) {
	*index = HASH2INDEX(hash);

	do {
		// If this entry is not occupied, the key is not in this table
		if (!session_hashtable.entries[*index].occupied)
			return 1;
		// If occupied and hash matches
		if (session_hashtable.entries[*index].hash == hash) {
			// check if the key also matches
			if (compare_five_tuples(&session_hashtable.entries[*index].key, key) == 0) {
				return 0;
			}
		}
		// otherwise, search the next entry
		*index = (*index + 1) % (SESSION_HASHTABLE_SIZE);
	} while (1);
}

/*
 * Get the value for a given key. Return 0 if key is in hashtable, 1 otherwise.
 * Value can be NULL if it should be ignored.
 */
int session_hashtable_get(const five_tuple_t *key, int32_t *value) {
	int32_t index;
	uint32_t hash;
	session_hashtable_hash_key(key, &hash);
	int ret = session_get_hashtable_index(key, hash, &index);
	if (value != NULL && ret == 0) {
		*value = session_hashtable.entries[index].value;
	}
	return ret;
}

static void session_hashtable_remove_by_index(int32_t index, int32_t session_id) {
	session_hashtable.entries[index].occupied = false;
	MEMORY_BARRIER
	session_hashtable.entries[index].hash = 0;
	session_hashtable.num_entries--;
	session_hashtable.egress_feature_to_session_id[session_hashtable.entries[index].value +
											 session_hashtable.entries[index].key.session_id_egress_offset] = -1;
	session_hashtable.feature_index_to_entry_map[session_hashtable.entries[index].value] = -1;

	// We found the and removed the value, now move all entries, which could not be placed on their
	// correct position on insertion, to their real place or at least nearer to it.
	// This ensures that for a given key k with hash h at index h+i, all entries from h to h+i are
	// occupied.
	int32_t empty_index = index;
	while (1) {
		// This loop will terminate, as we have at least one unoccupied entry in the hashtable
		index = (index + 1) % (SESSION_HASHTABLE_SIZE);
		// If we encounter an unoccupied entry, we can stop as there are no more entries after this,
		// which were misplaced due to our entry.
		if (!session_hashtable.entries[index].occupied) {
			break;
		}
		// If we encounter an entry where the index does not match the hash,
		// move it to the previous free slot if that is a valid slot for this item
		if (index != HASH2INDEX(session_hashtable.entries[index].hash)) {
			if (CAN_MOVE_ITEM_TO_CURRENT_EMPTY(HASH2INDEX(session_hashtable.entries[index].hash), empty_index, index)) {
				session_hashtable.entries[index].occupied = false;
				MEMORY_BARRIER
				session_hashtable.entries[empty_index].hash = session_hashtable.entries[index].hash;
				session_hashtable.entries[empty_index].value = session_hashtable.entries[index].value;
				session_hashtable.entries[empty_index].key = session_hashtable.entries[index].key;
				session_hashtable.egress_feature_to_session_id[session_hashtable.entries[index].value +
														 session_hashtable.entries[index].key.session_id_egress_offset] = session_hashtable.entries[index].value;
				session_hashtable.feature_index_to_entry_map[session_hashtable.entries[index].value] = empty_index;
				MEMORY_BARRIER
				session_hashtable.entries[empty_index].occupied = true;
				empty_index = index;
			}
		}
	}
}

/*
 * Remove a given key from the hash table. Return 0 if key is removed, 1 otherwise (not existent or flow count to high).
 * Value can be NULL if it should be ignored.
 */
int session_hashtable_remove_or_decrease_flow_count(const five_tuple_t *key, int32_t *value) {
	if (use_sessions()) {
		session_hashtable.flow_count[*value]--;

		// we don't really want to remove sessions that still have active flows
		if (session_hashtable.flow_count[*value] != 0)
			return 1;
	}

	int32_t index;
	int ret = 0;
	uint32_t hash;
	session_hashtable_hash_key(key, &hash);
//	LOCK(table);

	int status = session_get_hashtable_index(key, hash, &index);
	if (status) {
		ret = status;
	}
	else {
		*value = session_hashtable.entries[index].value;
		session_hashtable_remove_by_index(index, *value);
	}
//	UNLOCK(table);
	return ret;
}

int session_hashtable_remove_or_decrease_flow_count_by_value(const int32_t value, five_tuple_t *key) {
	if (use_sessions()) {
		session_hashtable.flow_count[value]--;

		// we don't really want to remove sessions that still have active flows
		if (session_hashtable.flow_count[value] != 0)
			return 1;
	}

	int ret = 0;
//	LOCK(table);
	int32_t index = session_hashtable.feature_index_to_entry_map[value];
	if (index == -1) {
		ret = 1;
	}
	else {
		if (key != NULL) {
			*key = session_hashtable.entries[index].key;
		}
		session_hashtable_remove_by_index(index, value);
	}
//	UNLOCK(table);
	return ret;
}

void session_hashtable_copy(session_hashtable_t *dest) {
//	LOCK(table);
	memcpy(dest, &session_hashtable, sizeof(session_hashtable_t));
//	UNLOCK(table);
}
