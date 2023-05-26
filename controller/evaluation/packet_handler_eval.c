#include "../controller.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../datastructures/packet_ringbuffer.h"
#include "../datastructures/dataplane_ringbuffer.h"
#include "../datastructures/dns_ringbuffer.h"
#include "../datastructures/flow_hashtable.h"
#include "../datastructures/bloom_filter.h"

#include "../dataplane/features.h"
#include "../flow_db/flow_db.h"
#include "../dataplane/prepare_dataplane.h"
#include "../packet_handler/packet_handler.h"
#include "../logging/statistics.h"

#define THREAD_CREATE_OR_EXIT(st, fn)                                                  \
    DEBUG_LOG("starting thread " #fn)                                                  \
    pthread_t st;                                                                      \
    if (pthread_create(&(st), NULL, fn, NULL) != 0) {                                  \
        DEBUG_LOG("Error starting thread " #fn "errno %d %s", errno, strerror(errno))  \
        exit(1);                                                                       \
    }

void add_relevant_dns_entry() {
	const uint32_t ttl = 100000;
	struct timespec now;
	dns_hashtable_key_t key = {0};
	// this is the ipv4 key mock_packets uses
	key.type = L3_IPV4;
	key.ipv4 = 16777343;
	VERBOSE_LOG("Adding dns key: %d", key.ipv4);
	clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
	dns_hashtable_add_or_change(&key, now.tv_sec + ttl);
}

void low_level_controller() {
	dataplane_new_features = false;
	pthread_cond_init(&dataplane_new_features_cond, NULL);
	pthread_mutex_init(&dataplane_new_features_lock, NULL);
	pthread_spin_init(&dataplane_feature_lock, 0);
	pthread_spin_init(&uuid_mapping_lock, 0);

	packet_ringbuffer_init();
	dataplane_ringbuffer_init();
	dns_ringbuffer_init();
	flow_hashtable_init();
	dns_hashtable_init();
	bloom_filter_init();

	add_relevant_dns_entry();

	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);
	prepare_dataplane(sess_hdl);
	prepare_features(sess_hdl);

	THREAD_CREATE_OR_EXIT(statistics, statistics_thread)
	sleep(1);
	THREAD_CREATE_OR_EXIT(packet_handler, packet_thread)
}