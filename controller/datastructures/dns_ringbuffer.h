#ifndef CONTROLLER_DNS_RINGBUFFER_H
#define CONTROLLER_DNS_RINGBUFFER_H

#include <stdint.h>

#include "mk_ringbuffer.h"
#include "dns_hashtable.h"
#include "../logging/statistics.h"

#define DNS_RINGBUFFER_SIZE 1 << 12
#define MAX_DNS_PKT_LEN 2048

typedef struct {
	union {
		struct {
			int32_t len;
			uint8_t pkt[MAX_DNS_PKT_LEN];
		} pkt;
		dns_hashtable_key_t address;
	};
	enum {
		DNS_ADD_PKT,
		DNS_REMOVE_IP,
	} command;
} dns_item_t;

GLOBAL_STAT_HDR(dns_ringbuffer)

MK_RINGBUFFER_HEADER(dns, dns_item_t, DNS_RINGBUFFER_SIZE)

#endif //CONTROLLER_DNS_RINGBUFFER_H


