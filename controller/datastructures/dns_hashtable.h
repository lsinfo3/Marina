#ifndef CONTROLLER_DNS_HASHTABLE_H
#define CONTROLLER_DNS_HASHTABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "../config.h"
#include "structs.h"

typedef struct {
	l3_proto_t type;
	union {
		uint32_t ipv4;
		struct in6_addr ipv6;
	};
} dns_hashtable_key_t;

typedef int64_t dns_hashtable_value_t;

typedef struct {
	dns_hashtable_key_t key;
	dns_hashtable_value_t value;
	uint32_t hash;
	bool occupied;
} dns_hashtable_entry_t;

typedef struct {
	size_t num_entries;
	dns_hashtable_entry_t entries[DNS_HASHTABLE_SIZE];
} dns_hashtable_t;


void dns_hashtable_init();

int dns_hashtable_add_or_change(const dns_hashtable_key_t *key, dns_hashtable_value_t value);

int dns_hashtable_get(const dns_hashtable_key_t *key, dns_hashtable_value_t *value);

int dns_hashtable_remove(const dns_hashtable_key_t *key);

#endif //CONTROLLER_DNS_HASHTABLE_H
