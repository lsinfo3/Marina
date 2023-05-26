#include <string.h>
#include <pcap.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdlib.h>
#include <resolv.h>

#include "packet_handler.h"

#include "../datastructures/flow_hashtable.h"
#include "../datastructures/dns_ringbuffer.h"
#include "../datastructures/packet_ringbuffer.h"

#ifdef MOCK_PACKETS
#include "../tests/mock_packets.h"
#endif

#define MAX_PKT_LEN 2048

#define ETH_TO_TS(buffer) (uint64_t)(buffer)[0] << 40 |   \
                          (uint64_t)(buffer)[1] << 32 |   \
                          (uint64_t)(buffer)[2] << 24 |   \
                          (uint64_t)(buffer)[3] << 16 |   \
                          (uint64_t)(buffer)[4] << 8  |   \
                          (uint64_t)(buffer)[5];

#define ETHERTYPE_IPV4_BE 0x0008
#define ETHERTYPE_IPV6_BE 0xdd86

pcap_t *pcap_descr = NULL;

GLOBAL_STAT_IMPL(packet_handler)

void hexDump(const char *desc, const void *addr, const int len) {
	int i;
	unsigned char buff[17];
	const unsigned char *pc = (const unsigned char *) addr;

	// Output description if given.

	if (desc != NULL)
		printf("%s:\n", desc);

	// Length checks.
	if (len == 0) {
		printf("  ZERO LENGTH\n");
		return;
	}
	else if (len < 0) {
		printf("  NEGATIVE LENGTH: %d\n", len);
		return;
	}

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).
		if ((i % 16) == 0) {
			// Don't print ASCII buffer for the "zeroth" line.
			if (i != 0)
				printf("  %s\n", buff);
			// Output the offset.
			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And buffer a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}
	// And print the final ASCII buffer.
	printf("  %s\n", buff);
}

/*
 * Return true if five tuple is already known
 */
static inline bool is_known(const five_tuple_t five_tuple) {
	return flow_hashtable_get(&five_tuple, NULL) == 0;

}

static inline dns_hashtable_key_t toDnsHashtableKey(const address_t address, const l3_proto_t proto) {
	dns_hashtable_key_t key = {0};

	key.type = proto;
	switch (proto) {
		case L3_IPV4:
			key.ipv4 = address.ipv4;
			break;
		case L3_IPV6:
			memcpy(&key.ipv6, address.ipv6.s6_addr, sizeof(struct in6_addr));
			break;
	}

	return key;
}

static inline bool isInDnsHashtable(const address_t address, const l3_proto_t proto) {
#ifdef USE_DNS_DB
	dns_hashtable_key_t key = toDnsHashtableKey(address, proto);
	return dns_hashtable_get(&key, NULL) == 0;
#else
	return true;
#endif
}

/*
 * Implement flow classification heuristic
 */
bool heuristic(const five_tuple_t five_tuple) {
	dns_hashtable_value_t timeout;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_COARSE, &now);

	dns_hashtable_key_t src_key = toDnsHashtableKey(five_tuple.h.src_addr, five_tuple.h.l3_proto);
	if (dns_hashtable_get(&src_key, &timeout) == 0) {
		if (timeout < now.tv_sec) {
			dns_item_t command = {
					.address = src_key,
					.command = DNS_REMOVE_IP,
			};
			dns_ringbuffer_put(&command);
			VERBOSE_LOG("Heuristic Timeout!");
		}
		else {
			VERBOSE_LOG("Heuristic Match!");
			return true;
		}
	}

	dns_hashtable_key_t dst_key = toDnsHashtableKey(five_tuple.h.dst_addr, five_tuple.h.l3_proto);
	if (dns_hashtable_get(&dst_key, &timeout) == 0) {
		if (timeout < now.tv_sec) {
			dns_item_t command = {
					.address = dst_key,
					.command = DNS_REMOVE_IP,
			};
			dns_ringbuffer_put(&command);
			VERBOSE_LOG("Heuristic Timeout!");
		}
		else {
			VERBOSE_LOG("Heuristic Match!");
			return true;
		}
	}
#ifdef USE_DNS_DB
	VERBOSE_LOG("No Heuristic Match!");
	return false;
#else
	VERBOSE_LOG("Hacked Heuristic Match!");
	return true;
#endif
}

static void setup_session_hash(five_tuple_t* five_tuple) {
	if (isInDnsHashtable(five_tuple->h.src_addr, five_tuple->h.l3_proto)) {
		five_tuple->sh.to_server = false;
		five_tuple->sh.client_addr = five_tuple->h.dst_addr;
	}
	else if (isInDnsHashtable(five_tuple->h.dst_addr, five_tuple->h.l3_proto)) {
		five_tuple->sh.to_server = true;
		five_tuple->sh.client_addr = five_tuple->h.src_addr;
	}
}

/*
 * processes a parsed packet by determining its flow status and sends corresponding commands
 * to the flow database
 */
static void process_packet(const five_tuple_t five_tuple, const struct tcphdr *tcpHeader) {
	packet_item_t command = {
			.args.five_tuple = five_tuple
	};
	if (five_tuple.h.l4_proto == L4_TCP) {
		if ((tcpHeader->th_flags & TH_SYN) && is_known(five_tuple)) {
			VERBOSE_LOG("SYN!")
			// Legacy case, we don't care anymore if a flow is reset (this does not reset the session)
			return;
		}
		else if (tcpHeader->th_flags & TH_FIN) {
			VERBOSE_LOG("FIN!")
			// We received a TCP FIN packet
			// Flush and remove this flow from our database
			command.command = PKT_REMOVE_FLOW;
			packet_ringbuffer_put(&command);
			return;
		}
	}

	if (heuristic(five_tuple)) {
		VERBOSE_LOG("New Flow!! dev_port: %d", five_tuple.dev_port)
		command.command = PKT_ADD_FLOW;
		packet_ringbuffer_put(&command);
		if (five_tuple.h.l4_proto == L4_UDP) {
			command.command = PKT_FIX_BLOOM_FILTER_RELEVANT;
			packet_ringbuffer_put(&command);
		}
	}
	else if (five_tuple.h.l4_proto == L4_UDP) {
		VERBOSE_LOG("UDP ignore!")
		command.command = PKT_ADD_BLOOM_FILTER_IRRELEVANT;
		packet_ringbuffer_put(&command);
	}
}

/*
 * Extract the next header from the packet, return if the header is longer than
 * the remaining packet
 */
#define EXTRACT(dest, type)                                  \
    if ((curSize + sizeof(type)) > pkthdr->caplen) return;   \
    (dest) = (type *) (packet + curSize);                    \
    curSize += sizeof(type);


/*
 * gets called from the pcap_loop function for each received packet
 * parses the ethernet, L3 and L4 headers and calls process_packet for each relevant packet
 */
static void
packet_handler(u_char *arg __attribute__((unused)), const struct pcap_pkthdr *pkthdr, const u_char *packet) {
	INC_STAT(packet_handler)

	five_tuple_t five_tuple;
	memset(&five_tuple.h, 0, sizeof(five_tuple_for_hash_t));
	memset(&five_tuple.sh, 0, sizeof(five_tuple_for_session_hash_t));

	struct ether_header *etherHeader = NULL;
	struct iphdr *ipHeader = NULL;
	struct ip6_hdr *ip6Header = NULL;
	struct tcphdr *tcpHeader = NULL;
	struct udphdr *udpHeader = NULL;

	size_t curSize = 0;

	EXTRACT(etherHeader, struct ether_header)

	// The dev_port is saved in the last 2 of the 6 bytes of the src mac
	five_tuple.dev_port = SWAP_ENDIAN_16(((uint16_t *) etherHeader->ether_shost)[2]);
	// The ingress timestamp is in the dst mac
	five_tuple.ingress_ts = ETH_TO_TS(etherHeader->ether_dhost);

	VERBOSE_LOG("New packet of type %d on port %d", etherHeader->ether_type, five_tuple.dev_port)
#ifdef VERBOSE
	hexDump(NULL, etherHeader, sizeof(struct ether_header));
#endif

	uint8_t l4Proto;

	switch (etherHeader->ether_type) {
		case ETHERTYPE_IPV4_BE:
			VERBOSE_LOG("IPv4!")
		EXTRACT(ipHeader, struct iphdr)
#ifdef VERBOSE
			hexDump(NULL, ipHeader, sizeof(struct iphdr));
#endif
			five_tuple.h.l3_proto = L3_IPV4;
			five_tuple.h.dst_addr.ipv4 = ipHeader->daddr;
			five_tuple.h.src_addr.ipv4 = ipHeader->saddr;
			l4Proto = ipHeader->protocol;
			break;
		case ETHERTYPE_IPV6_BE:
			VERBOSE_LOG("IPv6!")
		EXTRACT(ip6Header, struct ip6_hdr)
#ifdef VERBOSE
			hexDump(NULL, ip6Header, sizeof(struct ip6_hdr));
#endif
			five_tuple.h.l3_proto = L3_IPV6;
			five_tuple.h.dst_addr.ipv6 = ip6Header->ip6_dst;
			five_tuple.h.src_addr.ipv6 = ip6Header->ip6_src;
			l4Proto = ip6Header->ip6_nxt;
			break;
		default:
			return;
	}

	switch (l4Proto) {
		case IPPROTO_TCP:
			VERBOSE_LOG("TCP!")
		EXTRACT(tcpHeader, struct tcphdr)
#ifdef VERBOSE
			hexDump(NULL, tcpHeader, sizeof(struct tcphdr));
#endif
			five_tuple.h.dst_port = SWAP_ENDIAN_16(tcpHeader->dest);
			five_tuple.h.src_port = SWAP_ENDIAN_16(tcpHeader->source);
			five_tuple.h.l4_proto = L4_TCP;
			break;
		case IPPROTO_UDP:
			VERBOSE_LOG("UDP!")
		EXTRACT(udpHeader, struct udphdr)
#ifdef VERBOSE
			hexDump(NULL, udpHeader, sizeof(struct udphdr));
#endif
			five_tuple.h.dst_port = SWAP_ENDIAN_16(udpHeader->dest);
			five_tuple.h.src_port = SWAP_ENDIAN_16(udpHeader->source);
			five_tuple.h.l4_proto = L4_UDP;
			break;
		default:
			return;
	}

	if (l4Proto == IPPROTO_UDP && udpHeader->source == 53) { // DNS Response!!
		int32_t len = MAX(pkthdr->caplen - curSize, MAX_DNS_PKT_LEN); // FIXME
		if (len < NS_HFIXEDSZ) {
			return;
		}
		dns_item_t dns_packet = {
				.pkt.len = len,
				.command = DNS_ADD_PKT,
		};
		memcpy(dns_packet.pkt.pkt, packet + curSize, len);
		dns_ringbuffer_put(&dns_packet);
		return;
	}

	setup_session_hash(&five_tuple);

	process_packet(five_tuple, tcpHeader);
}

/*
 * executes the pcap_loop function which listens on the interface given in args
 */
#ifdef MOCK_PACKETS
_Noreturn
#endif

void *packet_thread(void *args __attribute__((unused))) {
#ifdef MOCK_PACKETS
	//	for (int i=0; i<3; i++) {mock_packets();}exit(0);
	//	while (1) { mock_packets(); }
		while (1) { mock_raw_packets(packet_handler); }
#else
	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0, PCAP_ERRBUF_SIZE);

	DEBUG_LOG("Opening network interface")
	if ((pcap_descr = pcap_open_live(DP_IFACE, MAX_PKT_LEN, 1, 512, errbuf)) == NULL) {
		printf("%s", errbuf);
		exit(1);
	}
	DEBUG_LOG("pcap_loop")
	pcap_loop(pcap_descr, -1, packet_handler, NULL);
	return NULL;
#endif
}