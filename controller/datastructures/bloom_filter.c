#include "bloom_filter.h"

#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "dataplane_ringbuffer.h"
#include "hashes/crc.h"

typedef struct __attribute__((__packed__)) {
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
} __attribute__((__packed__)) dataplane_five_tuple_ipv4_t;

typedef struct __attribute__((__packed__)) {
	uint8_t src_addr[16];
	uint8_t dst_addr[16];
	uint16_t src_port;
	uint16_t dst_port;
} __attribute__((__packed__)) dataplane_five_tuple_ipv6_t;

typedef struct {
	union __attribute__((__packed__)) {
		dataplane_five_tuple_ipv4_t ipv4;
		dataplane_five_tuple_ipv6_t ipv6;
	} __attribute__((__packed__)) dp;
	int len;
} dataplane_five_tuple_t;

struct {
	uint16_t entries[NUM_PIPES][NUM_BLOOMFILTERS][BLOOMFILTER_WIDTH];
	pthread_spinlock_t locks[NUM_PIPES][NUM_BLOOMFILTERS];
} bloom_filter;


void bloom_filter_init() {
	for (int pipe = 0; pipe < NUM_PIPES; pipe++) {
		for (int i = 0; i < NUM_BLOOMFILTERS; i++) {
			pthread_spin_init(&bloom_filter.locks[pipe][i], 0);
			for (int j = 0; j < BLOOMFILTER_WIDTH; j++) {
				bloom_filter.entries[pipe][i][j] = 0;
			}
		}
	}
}


//static void get_dataplane_fivetuple(const five_tuple_t *five_tuple, dataplane_five_tuple_t *dataplane_five_tuple) {
//	switch (five_tuple->h.l3_proto) {
//		case L3_IPV4:
//			dataplane_five_tuple->dp.ipv4.src_addr = five_tuple->h.src_addr.ipv4;
//			dataplane_five_tuple->dp.ipv4.dst_addr = five_tuple->h.dst_addr.ipv4;
//			dataplane_five_tuple->dp.ipv4.src_port = five_tuple->h.src_port;
//			dataplane_five_tuple->dp.ipv4.dst_port = five_tuple->h.dst_port;
//			dataplane_five_tuple->len = sizeof(dataplane_five_tuple_ipv4_t);
//			break;
//		case L3_IPV6:
//			memcpy(dataplane_five_tuple->dp.ipv6.src_addr, &five_tuple->h.src_addr.ipv6,
//				   sizeof(dataplane_five_tuple->dp.ipv6.src_addr));
//			memcpy(dataplane_five_tuple->dp.ipv6.dst_addr, &five_tuple->h.dst_addr.ipv6,
//				   sizeof(dataplane_five_tuple->dp.ipv6.dst_addr));
//			dataplane_five_tuple->dp.ipv6.src_port = five_tuple->h.src_port;
//			dataplane_five_tuple->dp.ipv6.dst_port = five_tuple->h.dst_port;
//			dataplane_five_tuple->len = sizeof(dataplane_five_tuple_ipv6_t);
//			break;
//	}
//}
//
//static uint32_t get_hash(int index, const dataplane_five_tuple_t *dataplane_five_tuple) {
//	return crc_fn[index]((uint8_t *) &dataplane_five_tuple->dp, dataplane_five_tuple->len, 0);
//}

typedef struct __attribute__((__packed__)) {
	uint32_t src_addr;
	uint32_t dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
} dp_five_tuple_t;

inline void mk_ipv4_hash(int *hashes, const five_tuple_t *five_tuple) {
	dp_five_tuple_t ft = {
			.src_addr = five_tuple->h.src_addr.ipv4,
			.dst_addr = five_tuple->h.dst_addr.ipv4,
			.src_port = SWAP_ENDIAN_16(five_tuple->h.src_port),
			.dst_port = SWAP_ENDIAN_16(five_tuple->h.dst_port),
	};
	for (int i = 0; i < NUM_BLOOMFILTERS; i++) {
		uint32_t hash = crc_fn[i]((uint8_t *) &ft, sizeof(dp_five_tuple_t), 0);
		hashes[i] = (int) (hash % BLOOMFILTER_WIDTH);
	}
}

inline void mk_ipv6_hash(int *hashes, const five_tuple_t *five_tuple) {
	dp_five_tuple_t ft = {
			.src_addr = crc_fn[0]((uint8_t *) &five_tuple->h.src_addr.ipv6.s6_addr, 16, 0),
			.dst_addr = crc_fn[0]((uint8_t *) &five_tuple->h.dst_addr.ipv6.s6_addr, 16, 0),
			.src_port = SWAP_ENDIAN_16(five_tuple->h.src_port),
			.dst_port = SWAP_ENDIAN_16(five_tuple->h.dst_port),
	};
	for (int i = 0; i < NUM_BLOOMFILTERS; i++) {
		uint32_t hash = crc_fn[i]((uint8_t *) &ft, sizeof(dp_five_tuple_t), 0);
		hashes[i] = (int) (hash % BLOOMFILTER_WIDTH);
	}
}


inline void get_hashes(int *hashes, const five_tuple_t *five_tuple) {
	switch (five_tuple->h.l3_proto) {
		case L3_IPV4:
			mk_ipv4_hash(hashes, five_tuple);
			break;
		case L3_IPV6:
			mk_ipv6_hash(hashes, five_tuple);
			break;
	}
}

void bloom_filter_set_irrelevant(const five_tuple_t *five_tuple) {
	int pipe = dev_port_to_pipe(five_tuple->dev_port);

	dataplane_command_t command = {
			.args.bloom_filter_cmd = {
					.pipe_id = pipe,
			},
			.command = DP_ADD_BLOOM_FILTER,
	};

	int indices[NUM_BLOOMFILTERS];
	get_hashes(indices, five_tuple);

	bool send = false;
	for (int i = 0; i < NUM_BLOOMFILTERS; i++) {
		int index = indices[i];

		uint16_t *entry = &bloom_filter.entries[pipe][i][index];
		pthread_spin_lock(&bloom_filter.locks[pipe][i]);
		(*entry)++;
		pthread_spin_unlock(&bloom_filter.locks[pipe][i]);

		if (*entry == 1) {
			command.args.bloom_filter_cmd.bloom_filter_indices[i] = index;
			send = true;
		}
	}

	if (send) {
		dataplane_ringbuffer_put(&command);
	}
}

void bloom_filter_set_relevant(const five_tuple_t *five_tuple) {
	int pipe = dev_port_to_pipe(five_tuple->dev_port);
	dataplane_command_t command = {
			.args.bloom_filter_cmd = {
					.pipe_id = pipe,
			},
			.command = DP_REMOVE_BLOOM_FILTER,
	};

	int indices[NUM_BLOOMFILTERS];
	get_hashes(indices, five_tuple);

	bool send = false;
	for (int i = 0; i < NUM_BLOOMFILTERS; i++) {
		int index = indices[i];
//		if (i == 0) {
//			char ip[INET_ADDRSTRLEN];
//			inet_ntop(AF_INET, (void *) &five_tuple->h.src_addr.ipv4, ip, INET_ADDRSTRLEN);
//			printf("%s ", ip);
//			inet_ntop(AF_INET, (void *) &five_tuple->h.dst_addr.ipv4, ip, INET_ADDRSTRLEN);
//			printf("%s ", ip);
//			printf("%d %d: %x\n", five_tuple->h.src_port, five_tuple->h.dst_port, index);
//		}

		if (bloom_filter.entries[pipe][i][index] == 0) {
			command.args.bloom_filter_cmd.bloom_filter_indices[i] = index;
			send = true;
		}
	}
	if (send) {
		dataplane_ringbuffer_put(&command);
	}
}

#define sub_threshold ((int)(RAND_MAX * BLOOM_SUB_PROBABILITY))

_Noreturn void *bloom_filter_maintenance(__attribute__((unused)) void *arg) {
	int index, pipe, i;
	dataplane_command_t command = {
			.command = DP_REMOVE_BLOOM_FILTER,
	};
	bool send = false;
	BENCH_PREPARE(bloom_maint)
	while (1) {
		usleep(1000 * 1000);
		BENCH_START(bloom_maint)
		for (pipe = 0; pipe < NUM_PIPES; pipe++) {
			command.args.bloom_filter_cmd.pipe_id = pipe;
			for (index = 0; index < BLOOMFILTER_WIDTH; index++) {
				for (i = 0; i < NUM_BLOOMFILTERS; i++) {
					if (random() < sub_threshold) {
						if (bloom_filter.entries[pipe][i][index] == 0) {
							continue;
						}
						ERROR_LOG("Bloom filter maint %d %d", index, i)
						pthread_spin_lock(&bloom_filter.locks[pipe][i]);
						if (bloom_filter.entries[pipe][i][index] > 1) {
							bloom_filter.entries[pipe][i][index]--;
							pthread_spin_unlock(&bloom_filter.locks[pipe][i]);
						}
						else if (bloom_filter.entries[pipe][i][index] == 1) {
							bloom_filter.entries[pipe][i][index]--;
							pthread_spin_unlock(&bloom_filter.locks[pipe][i]);
							command.args.bloom_filter_cmd.bloom_filter_indices[i] = index;
							send = true;
						}
					}
				}
				if (send) {
					send = false;
					if (dataplane_ringbuffer_put(&command) != 0) {
						for (i = 0; i < NUM_BLOOMFILTERS; i++) {
							pthread_spin_lock(&bloom_filter.locks[pipe][i]);
							bloom_filter.entries[pipe][i][command.args.bloom_filter_cmd.bloom_filter_indices[i]]++;
							pthread_spin_unlock(&bloom_filter.locks[pipe][i]);
						}
					}
				}
			}
		}
		BENCH_END(bloom_maint, "Bloom filter maintenance took")
	}
}
