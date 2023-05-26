#ifndef LOW_LEVEL_LIB_STRUCTS_H
#define LOW_LEVEL_LIB_STRUCTS_H

#include <stdint.h>
#include <netinet/ip6.h>
#include <pipe_mgr/pipe_mgr_intf.h>

#include <byteswap.h>

typedef enum {
	L3_IPV4 = AF_INET,
	L3_IPV6 = AF_INET6,
} l3_proto_t;

typedef enum {
	L4_TCP = 1,
	L4_UDP = 0,
} l4_proto_t;

typedef uint16_t port_t;

typedef union {
	uint32_t ipv4;
	struct in6_addr ipv6;
} address_t;

typedef struct {
	address_t src_addr;
	address_t dst_addr;
	port_t src_port;
	port_t dst_port;
	l3_proto_t l3_proto;
	l4_proto_t l4_proto;
} five_tuple_for_hash_t;

// we are only interested in the client the communicates with the youtube server and the direction
typedef struct  {
	address_t client_addr;
	bool to_server;
} five_tuple_for_session_hash_t;

typedef struct {
	five_tuple_for_hash_t h;
	five_tuple_for_session_hash_t sh;
	uint64_t ingress_ts;
	uint16_t dev_port;
	int8_t session_id_egress_offset;
} five_tuple_t;

#ifdef RUN_ON_MODEL
#define dev_port_to_pipe(dev_port) (dev_port_to_pipe_id(dev_port))
#else
#define dev_port_to_pipe(dev_port) ((dev_port) >> 7)
#endif

#define flow_id_to_pipe(flow_id) ((flow_id) % 4)

#define flow_id_to_register_index(flow_id) ((flow_id) / 4)

#define SWAP_ENDIAN_16(x) bswap_16(x)

#define SWAP_ENDIAN_32(x) bswap_32(x)

#endif //LOW_LEVEL_LIB_STRUCTS_H
