#include "dns_db.h"

#include <resolv.h>
#include <arpa/inet.h>

#include "../config.h"
#include "../dns_config.h"
#include "../datastructures/dns_ringbuffer.h"

FILE *dns_pipe;

GLOBAL_STAT_IMPL(dnsdb)

/*
 * Export an ip address including its ttl to the dns pipe to be read by the qoe pipeline
 */
static inline void export_key(dns_hashtable_key_t key, uint32_t type, uint32_t ttl) {
	char buf[INET6_ADDRSTRLEN];
	switch (type) {
		case ns_t_a:
			inet_ntop(AF_INET, (void *) &key.ipv4, buf, INET_ADDRSTRLEN);
			break;
		case ns_t_aaaa:
			inet_ntop(AF_INET6, (void*) &key.ipv6, buf, INET6_ADDRSTRLEN);
			break;
		default:
			return;
	}
	fprintf(dns_pipe, "%s,%d\n", buf, ttl);
	fflush(dns_pipe);
}

/*
 * Parse a dns packet and insert the matching ip addresses in the answer sections into
 * the dns hashtable
 */
static inline void parse_dns_packet(uint8_t *pkt, int32_t len) {
	ns_msg msg_hdl;
	uint32_t rr_count;
	uint32_t type, ttl, name_len, entry_len;
	char *name;
	ns_rr rr;
	const uint8_t *rrdata;
	dns_hashtable_key_t key = {0};

	// Prepare msg_hdl
	if (ns_initparse(pkt, len, &msg_hdl) < 0) {
		return;
	}

	// Count the resource records in the answer section
	rr_count = ns_msg_count(msg_hdl, ns_s_an);
	if (rr_count == 0) {
		return;
	}

	for (int rr_num = 0; rr_num < rr_count; rr_num++) {
		if (ns_parserr(&msg_hdl, ns_s_an, rr_num, &rr)) {
			return;
		}

		type = ns_rr_type(rr);
		if (!(type == ns_t_a) && !(type == ns_t_aaaa)) continue;

		name = ns_rr_name(rr);
		name_len = strnlen(name, NS_MAXDNAME);
		// Check each entry in relevant domain names for a match
		// If answer is in this list, then insert the ip address
		// into the hashtable and print it out to the dns_pipe
		for (int entry = 0; entry < sizeof(RELEVANT_DOMAIN_NAMES); entry++) {
			entry_len = strnlen(RELEVANT_DOMAIN_NAMES[entry], MAX_DNSDB_DOMAIN_LENGTH);
			if (entry_len <= name_len) {
				if (strncmp(name + (name_len - entry_len), RELEVANT_DOMAIN_NAMES[entry], entry_len) == 0) {
					rrdata = ns_rr_rdata(rr);
					switch (type) {
						case ns_t_a:
							memcpy(&key.ipv4, rrdata, sizeof(uint32_t));
							break;
						case ns_t_aaaa:
							memcpy(&key.ipv6.s6_addr, rrdata, sizeof(struct in6_addr));
							break;
						default:
							return;
					}
					struct timespec now;
					clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
					ttl = ns_rr_ttl(rr);
					dns_hashtable_add_or_change(&key, now.tv_sec + ttl);
					export_key(key, type, ttl);
					return;
				}
			}
		}
	}
}

_Noreturn void *dns_db_thread(void *arg __attribute__((unused))) {
	DEBUG_LOG("Starting dns_db thread")
	dns_item_t packet;

	dns_pipe = fopen(DNS_PIPE, "w");

	while (1) {
		while (dns_ringbuffer.empty);
		while (dns_ringbuffer_get(&packet) == 0) {
			VERBOSE_LOG("Got DNS packet")
			INC_STAT(dnsdb)
			switch(packet.command) {
				case DNS_ADD_PKT:
					parse_dns_packet(packet.pkt.pkt, packet.pkt.len);
					break;
				case DNS_REMOVE_IP:
					dns_hashtable_remove(&packet.address);
					break;
			}
		}
	}
}