#include <pthread.h>
#include <unistd.h>

#include "../export/export_features.h"
#include "../dataplane/features.h"
#include "../datastructures/flow_hashtable.h"
#include "../flow_db/flow_db.h"

#define NUM_STEPS 20

int main() {
	dataplane_new_features = false;
	pthread_cond_init(&dataplane_new_features_cond, NULL);
	pthread_mutex_init(&dataplane_new_features_lock, NULL);
	pthread_spin_init(&dataplane_feature_lock, 0);
	pthread_spin_init(&uuid_mapping_lock, 0);

	flow_hashtable_init();

	pthread_t thread;
	pthread_create(&thread, NULL, feature_export_thread, NULL);

	five_tuple_t five_tuple = {
			.h = {
					.dst_port = 1234,
					.src_port = 1234,
					.dst_addr.ipv4 = 1,
					.src_addr.ipv4 = 1234,
					.l3_proto = L3_IPV4,
					.l4_proto = L4_TCP,
			},
			.session_id_egress_offset = 0,
			.dev_port = 1,
			.ingress_ts = 1,
	};

	int flow_id_counter = 0;

	for (int i = 0; i < NUM_STEPS; i++) {
		for (int j = 0; j < MAX_FLOWS * NUM_PIPES / NUM_STEPS; j++) {
			five_tuple.h.dst_addr.ipv4 = flow_id_counter;
			flow_hashtable_add(&five_tuple, flow_id_counter);
			flow_id_counter++;
		}
		for (int j=0; j< 50; j++) {
			INFO_LOG("Flow hashtable has %d entries", flow_id_counter);
			pthread_mutex_lock(&dataplane_new_features_lock);
			dataplane_new_features = true;
			pthread_cond_signal(&dataplane_new_features_cond);
			pthread_mutex_unlock(&dataplane_new_features_lock);
			sleep(2);
            fflush(stdout);
		}
	}

	pthread_join(thread, NULL);
	return 0;
}
