#include "dns_hashtable.h"

#include <string.h>
#include <stdatomic.h>

#include "hashes/MurmurHash3.h"


#if (DNS_HASHTABLE_SIZE & (DNS_HASHTABLE_SIZE - 1)) == 0
#define POSITIVE_MODULO(x) ((x) & (DNS_HASHTABLE_SIZE - 1))
#else
#warning "Using suboptimal operators, as DNS_HASHTABLE_SIZE is not a power of 2"
int32_t modulo(int32_t x) {
	x %= DNS_HASHTABLE_SIZE;
	if (x < 0) {
		x += DNS_HASHTABLE_SIZE;
	}
	return x;
}
#define POSITIVE_MODULO(x) modulo(x)
#endif

#define HASH2INDEX(hash) POSITIVE_MODULO(hash)
#define CAN_MOVE_ITEM_TO_CURRENT_EMPTY(item_desired_index, current_empty, item_index) \
    POSITIVE_MODULO((item_index) - (item_desired_index)) > \
    POSITIVE_MODULO((current_empty) - (item_desired_index))
#define MEMORY_BARRIER atomic_signal_fence(memory_order_acq_rel);

dns_hashtable_t dns_hashtable;

static inline void dns_hashtable_hash_key(const dns_hashtable_key_t *key, uint32_t *hash) {
	MurmurHash3_x86_32(key, sizeof(dns_hashtable_key_t), MURMUR_SEED, hash);
}

void dns_hashtable_init() {
	dns_hashtable.num_entries = 0;
	memset(dns_hashtable.entries, 0, sizeof(dns_hashtable_entry_t) * DNS_HASHTABLE_SIZE);
}

static inline int
get_hashtable_index(const dns_hashtable_key_t *key, const uint32_t hash, int32_t *index) {
	*index = HASH2INDEX(hash);

	do {
		// If this entry is not occupied, the key is not in this table
		if (!dns_hashtable.entries[*index].occupied)
			return 1;
		// If occupied and hash matches
		if (dns_hashtable.entries[*index].hash == hash) {
			// check if the key also matches
			if (memcmp(&dns_hashtable.entries[*index].key, key, sizeof(dns_hashtable_key_t)) == 0) {
				return 0;
			}
		}
		// otherwise, search the next entry
		*index = (*index + 1) % (DNS_HASHTABLE_SIZE);
	} while (1);
}

int dns_hashtable_add_or_change(const dns_hashtable_key_t *key, const dns_hashtable_value_t value) {
	if (dns_hashtable.num_entries == DNS_HASHTABLE_SIZE) {
		return 1;
	}
	uint32_t hash;
	int32_t index;
	dns_hashtable_hash_key(key, &hash);

	if (get_hashtable_index(key, hash, &index) == 0) {
		dns_hashtable.entries[index].value = value;
		return 0;
	}

	dns_hashtable.entries[index].key = *key;
	dns_hashtable.entries[index].value = value;
	dns_hashtable.entries[index].hash = hash;
	dns_hashtable.num_entries++;
	MEMORY_BARRIER
	dns_hashtable.entries[index].occupied = true;
	return 0;
}

int dns_hashtable_get(const dns_hashtable_key_t *key, dns_hashtable_value_t *value) {
	int32_t index;
	uint32_t hash;
	dns_hashtable_hash_key(key, &hash);
	int ret = get_hashtable_index(key, hash, &index);
	if (value != NULL && ret == 0) {
		*value = dns_hashtable.entries[index].value;
	}
	return ret;
}

int dns_hashtable_remove(const dns_hashtable_key_t *key) {
	int32_t index;
	int ret = 0;
	uint32_t hash;
	dns_hashtable_hash_key(key, &hash);

	int status = get_hashtable_index(key, hash, &index);
	if (status) {
		ret = status;
	}
	else {
		dns_hashtable.entries[index].occupied = false;
		MEMORY_BARRIER
		dns_hashtable.num_entries--;

		// We found the and removed the value, now move all entries, which could not be placed on their
		// correct position on insertion, to their real place or at least nearer to it.
		// This ensures that for a given key k with hash h at index h+i, all entries from h to h+i are
		// occupied.
		int32_t empty_index = index;
		while (1) {
			// This loop will terminate, as we have at least one unoccupied entry in the hashtable
			index = (index + 1) % (DNS_HASHTABLE_SIZE);
			// If we encounter an unoccupied entry, we can stop as there are no more entries after this,
			// which were misplaced due to our entry.
			if (!dns_hashtable.entries[index].occupied) {
				break;
			}
			// If we encounter an entry where the index does not match the hash,
			// move it to the previous free slot if that is a valid slot for this item
			if (index != HASH2INDEX(dns_hashtable.entries[index].hash)) {
				if (CAN_MOVE_ITEM_TO_CURRENT_EMPTY(HASH2INDEX(dns_hashtable.entries[index].hash), empty_index, index)) {
					dns_hashtable.entries[index].occupied = false;
					MEMORY_BARRIER
					dns_hashtable.entries[empty_index].hash = dns_hashtable.entries[index].hash;
					dns_hashtable.entries[empty_index].value = dns_hashtable.entries[index].value;
					dns_hashtable.entries[empty_index].key = dns_hashtable.entries[index].key;
					MEMORY_BARRIER
					dns_hashtable.entries[empty_index].occupied = true;
					empty_index = index;
				}
			}
		}
	}
	return ret;
}