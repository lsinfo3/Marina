#include <pthread.h>

#include "../datastructures/packet_ringbuffer.h"
#include "../datastructures/dataplane_ringbuffer.h"
#include "../flow_db/flow_db.h"
#include "../datastructures/flow_hashtable.h"
#include "../logging/statistics.h"
#include "../datastructures/bloom_filter.h"


int main() {
	flow_hashtable_init();
	packet_ringbuffer_init();
	dataplane_ringbuffer_init();
	pthread_spin_init(&uuid_mapping_lock, 0);
	bloom_filter_init();

	pthread_t flow_thread, stat_thread;
	pthread_create(&flow_thread, NULL, flowdb_thread, NULL);
	pthread_create(&stat_thread, NULL, statistics_thread, NULL);

	packet_item_t add_command = {
			.command = PKT_FIX_BLOOM_FILTER_RELEVANT,
			.args = {
					.five_tuple = {
							.h = {
									.dst_port = 1234,
									.src_port = 1234,
									.dst_addr.ipv4 = 1234,
									.src_addr.ipv4 = 1234,
									.l3_proto = L3_IPV4,
									.l4_proto = L4_TCP,
							},
							.session_id_egress_offset = 0,
							.dev_port = 1,
							.ingress_ts = 1,
					},
			},
	};

	packet_item_t remove_command = add_command;
	remove_command.command = PKT_ADD_BLOOM_FILTER_IRRELEVANT;

	for (int i=0; true; i++) {
		packet_ringbuffer_ensure_put(&add_command);
		packet_ringbuffer_ensure_put(&remove_command);
	}

	return 0;
}