#include <pthread.h>

#include "../dns_db/dns_db.h"
#include "../datastructures/dns_ringbuffer.h"
#include "../logging/statistics.h"


uint8_t dns_packet[] = \
"\xc0\x41\x81\x80\x00\x01\x00\x02" "\x00\x00\x00\x01\x10\x72\x33\x2d"     /*  0-15 */ \
"\x2d\x2d\x73\x6e\x2d\x68\x30\x6a" "\x65\x65\x6e\x65\x72\x0b\x67\x6f"     /* 16-31 */ \
"\x6f\x67\x6c\x65\x76\x69\x64\x65" "\x6f\x03\x63\x6f\x6d\x00\x00\x01"     /* 32-47 */ \
"\x00\x01\xc0\x0c\x00\x05\x00\x01" /*TTL(s):*/"\x00\x00\x00\x10" "\x00\x11\x02\x72"  /* 48-63 */ \
"\x33\x0b\x73\x6e\x2d\x68\x30\x6a" "\x65\x65\x6e\x65\x72\xc0\x1d\xc0"     /* 64-79 */ \
"\x3e\x00\x01\x00\x01\x00\x00\x01" "\x56\x00\x04" /*IP:*/"\xac\xd9\x85\x68" "\x00"  /* 80-95 */ \
"\x00\x29\x04\xd0\x00\x00\x00\x00" "\x00\x00";                            /* 96-105 */


#define SET_RESPONSE_IP(pkt, ip) *(uint32_t*)((pkt) + 91) = ip

int main() {
	dns_hashtable_init();
	dns_ringbuffer_init();
	pthread_t dns_thread, stat_thread;
	pthread_create(&dns_thread, NULL, dns_db_thread, NULL);
	pthread_create(&stat_thread, NULL, statistics_thread, NULL);

	dns_item_t add_command = {
			.command = DNS_ADD_PKT,
			.pkt.len = sizeof(dns_packet),
	};
	memcpy(add_command.pkt.pkt, dns_packet, sizeof(dns_packet));

	for (int i = 0; true; i++) {
		SET_RESPONSE_IP(add_command.pkt.pkt, i);
		dns_ringbuffer_ensure_put(&add_command);
	}

	return 0;
}