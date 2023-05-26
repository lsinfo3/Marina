#ifndef LOW_LEVEL_LIB_MOCK_PACKETS_H
#define LOW_LEVEL_LIB_MOCK_PACKETS_H

#include <sys/time.h>
#include <unistd.h>

#include "../datastructures/packet_ringbuffer.h"

void mock_packets() {
	VERBOSE_LOG("Injecting mock packet")
	five_tuple_t test = {
			.h = {
					.src_addr.ipv4 = 1234,
					.dst_addr.ipv4 = 1235,
					.src_port = 80,
					.dst_port = 90,
					.l3_proto = L3_IPV4,
					.l4_proto = L4_UDP,
			},
			.sh = {
					.client_addr.ipv4 = 1234,
					.to_server = 1,
			},
			.dev_port = 1
	};
	packet_item_t item = {
			.args.five_tuple = test,
			.command = PKT_ADD_FLOW
	};
	packet_ringbuffer_put(&item);
	VERBOSE_LOG("Ringbuffer size %d", packet_ringbuffer_length())
	usleep(3 * 1000 * 1000);
}

typedef void (*packet_handler_t)(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet);

static u_char raw_packet[255] = \
        "\x01\x01\x01\x01\x01\x01" "\x00\x00\x00\x00\x00\x00" "\x08\x00" \
        "\x45" "\x00"
		"\x00\x28\x00\x01\x00\x00\x40\x06\x7c\xcd\x7f\x00\x00\x01\x7f\x00" \
        "\x00\x01\x00\x14\x00\x50\x00\x00\x00\x00\x00\x00\x00\x00\x50\x02" \
        "\x20\x00\x91\x7c\x00\x00";
static struct pcap_pkthdr pkthdr = {
		.caplen = 255,
		.len = 255
};

void mock_raw_packets(packet_handler_t handler) {
	VERBOSE_LOG("Creating raw packet")
	// 26 is the pointer to the least significant bit of ipv4.src_addr
	raw_packet[26] += 1;
	handler(NULL, &pkthdr, raw_packet);
	//usleep(3 * 1000 * 1000);
}

#endif //LOW_LEVEL_LIB_MOCK_PACKETS_H
