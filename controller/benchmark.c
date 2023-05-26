#include "controller.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "datastructures/packet_ringbuffer.h"
#include "datastructures/dataplane_ringbuffer.h"
#include "datastructures/dns_ringbuffer.h"
#include "datastructures/flow_hashtable.h"
#include "datastructures/bloom_filter.h"

#include "dataplane/features.h"
#include "flow_db/flow_db.h"
#include "dataplane/prepare_dataplane.h"
#include "datastructures/session_hashtable.h"
#include "export/export_features.h"


#define NUM_INSERT_REMOVE (1l<<18)
#define NUM_BENCH_BLOOM (1l<<19)
#define NUM_BENCH_FEATURE (1l<<18)
#define NUM_K 50

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define BENCH_TO_OPS(name, num_ops) ((num_ops) * 1000l * 1000) / (_bench_##name##_diff.tv_sec * 1000 * 1000 + _bench_##name##_diff.tv_usec);

volatile bool run_feature_reader = true;


void *background_feature_reader(__attribute__((unused)) void *arg) {
	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);
	while (run_feature_reader) {
		read_features(sess_hdl);
	}
	switch_disconnect(sess_hdl);
	return NULL;
}

void *bench_classification_insert_remove(__attribute__((unused)) void *arg) {
	five_tuple_t five_tuple = {
			.h = {
					.l3_proto = L3_IPV4,
					.l4_proto = L4_TCP,
					.dst_port = 1,
					.src_port = 1,
					.dst_addr.ipv4 = 1,
			},
			.dev_port = 1,
			.ingress_ts = 1,
			.session_id_egress_offset = 0,
	};
	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);

	//p4_pd_entry_hdl_t entry_hdl;
	int64_t ops_per_sec;
	BENCH_PREPARE_FORCE(round)

	/*for (int k = 0; k < NUM_K; k++) {
		BENCH_START_FORCE(round)
		for (uint32_t i = 0; i < NUM_INSERT_REMOVE; i++) {
			five_tuple.h.src_addr.ipv4 = i;
			pipe_mgr_begin_batch(sess_hdl);
			switch_add_classification_entry(&five_tuple, sess_hdl, i, &entry_hdl);
			pipe_mgr_end_batch(sess_hdl, true);
			pipe_mgr_begin_batch(sess_hdl);
			switch_remove_classification_entry(sess_hdl, &five_tuple, entry_hdl);
			pipe_mgr_end_batch(sess_hdl, true);
		}
		BENCH_END_FORCE(round, "Inserting took")
		ops_per_sec = BENCH_TO_OPS(round, 2 * NUM_INSERT_REMOVE);
		INFO_LOG("Insert/Remove Op/s: %ld", ops_per_sec);
		fflush(stdout);
	}*/

	uint32_t count;
//	p4_pd_marina_data_plane_classification_ipv4_get_entry_count(sess_hdl, ALL_PIPES, &count);
//	ERROR_LOG("Number of entries: %u", count)

// don't know where magic number 8192 is from, but there were 4098 entries less
#define num_ops (MIN(MAX_FLOWS, MAX_IP4_FLOWS) - 8192 - 4098)

//	ERROR_LOG("Num ops: %d", num_ops)
//	return NULL;
	p4_pd_entry_hdl_t entry_hdls[num_ops];

	for (int k = 0; k < NUM_K; k++) {
		BENCH_START_FORCE(round)
		for (uint32_t i = 0; i < num_ops; i++) {
			five_tuple.h.src_addr.ipv4 = i;
			pipe_mgr_begin_batch(sess_hdl);
			if (switch_add_classification_entry(&five_tuple, sess_hdl, i, &entry_hdls[i]) == ERR_SWITCH) {
				pipe_mgr_end_batch(sess_hdl, true);
				p4_pd_marina_data_plane_classification_ipv4_get_entry_count(sess_hdl, ALL_PIPES, &count);
				ERROR_LOG("Number of entries: %u (%u)", count, i)
				return NULL;
			}
			pipe_mgr_end_batch(sess_hdl, true);
		}
		BENCH_END_FORCE(round, "Inserting took")
		ops_per_sec = BENCH_TO_OPS(round, num_ops);
		INFO_LOG("Insert Op/s: %ld", ops_per_sec);
		fflush(stdout);


		BENCH_START_FORCE(round)
		for (uint32_t i = 0; i < num_ops; i++) {
			five_tuple.h.src_addr.ipv4 = i;
			pipe_mgr_begin_batch(sess_hdl);
			switch_remove_classification_entry(sess_hdl, &five_tuple, entry_hdls[i]);
			pipe_mgr_end_batch(sess_hdl, true);
		}
		BENCH_END_FORCE(round, "Removing took")
		ops_per_sec = BENCH_TO_OPS(round, num_ops);
		INFO_LOG("Remove Op/s: %ld", ops_per_sec);
		fflush(stdout);
	}

	switch_disconnect(sess_hdl);
	return NULL;
}

void *bench_feature_reader(__attribute__((unused)) void *arg) {
	INFO_LOG("bench_feature_reader")

	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);
	BENCH_PREPARE_FORCE(round)

	for (int i = 0; i < 100; i++) {
		BENCH_START_FORCE(round)
		read_features(sess_hdl);
		BENCH_END_FORCE(round, "read_features")
	}
	fflush(stdout);

	return NULL;
}

void *bench_feature_export_data_copy(__attribute__((unused)) void *arg) {
	INFO_LOG("bench_feature_export_data_copy")
	static session_hashtable_t hashtable_copy;
	static char uuid_mapping_copy[FEATURE_SIZE][UUID_STR_LEN];
	BENCH_PREPARE_FORCE(round)

	for (int i = 0; i < 100; i++) {
		BENCH_START_FORCE(round)
		session_hashtable_copy(&hashtable_copy);
		memcpy(uuid_mapping_copy, uuid_mapping, sizeof(char) * FEATURE_SIZE * UUID_STR_LEN);
		BENCH_END_FORCE(round, "data_copy")
	}
	fflush(stdout);

	return NULL;
}

void *bench_feature_export_to_file(__attribute__((unused)) void *arg) {
	INFO_LOG("bench_feature_export_to_file")

	five_tuple_t five_tuple = {
			.h = {
					.l3_proto = L3_IPV4,
					.l4_proto = L4_TCP,
					.dst_port = 1234,
					.src_port = 4321,
					.dst_addr.ipv4 = 1,
			},
			.dev_port = 1,
			.ingress_ts = 1,
			.session_id_egress_offset = 0,
			.sh = {
					.client_addr = (address_t)(uint32_t)0,
					.to_server = true,
			}
	};

	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);

	STATIC_FEATURES_ARRAY
	static session_hashtable_t hashtable_copy;
	static char uuid_mapping_copy[FEATURE_SIZE][UUID_STR_LEN];
	session_hashtable_init();

	// create flows, so they can be exported
	int active_sessions_count = FEATURE_SIZE;
	for (int i = 0; i < active_sessions_count; i++) {
		address_t address1 = (address_t)(uint32_t)i;
		five_tuple.sh.client_addr = address1;
		five_tuple.h.src_addr = address1;
		int32_t session_id = i;
		session_hashtable_add_or_increase_flow_count(&five_tuple, session_id);
	}

	read_features(sess_hdl);
	session_hashtable_copy(&hashtable_copy);
	memcpy(uuid_mapping_copy, uuid_mapping, sizeof(char) * FEATURE_SIZE * UUID_STR_LEN);
	FILE* file = open_output_file();

	BENCH_PREPARE_FORCE(round)

	for (int i = 0; i < 100; i++) {
		BENCH_START_FORCE(round)
		print_header(file, features);
		print_flow_data(file, &hashtable_copy, features, uuid_mapping_copy);
		BENCH_END_FORCE(round, "print_features")
	}
	fflush(stdout);

	return NULL;
}

/*#define BLOOM_FILTER_STRIDE 10000

void *bench_bloom_filter(__attribute__((unused)) void *arg) {
	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);

	uint8_t unset = 0, set = 1;
	int64_t ops_per_sec;

	pipe_mgr_begin_batch(sess_hdl);
	for (int i = 0; i < BLOOM_FILTER_STRIDE; i++) {
		p4_pd_marina_data_plane_register_write_bloom_filter_1(
				sess_hdl, PIPES[0], i, &set
		);
	}
	pipe_mgr_end_batch(sess_hdl, true);

	BENCH_PREPARE_FORCE(bloom)

	int index = BLOOM_FILTER_STRIDE;
	for (int i = 0; i < NUM_K; i++) {
		BENCH_START_FORCE(bloom)
		for (int j = 0; j < NUM_BENCH_BLOOM; j++) {
			index++;
			pipe_mgr_begin_batch(sess_hdl);
			p4_pd_marina_data_plane_register_write_bloom_filter_1(
					sess_hdl, PIPES[0], (index) % BLOOMFILTER_WIDTH, &set
			);
			p4_pd_marina_data_plane_register_write_bloom_filter_1(
					sess_hdl, PIPES[0],
					(index - BLOOM_FILTER_STRIDE) % BLOOMFILTER_WIDTH, &unset
			);
			pipe_mgr_end_batch(sess_hdl, true);
		}
		BENCH_END_FORCE(bloom, "Bloom setting done")
		ops_per_sec = BENCH_TO_OPS(bloom, NUM_BENCH_BLOOM * 2);
		INFO_LOG("Bloom Op/s: %ld", ops_per_sec);
		fflush(stdout);
	}
	switch_disconnect(sess_hdl);
	return NULL;
}*/

void *bench_feature_reset(__attribute__((unused)) void *arg) {
	STATIC_FEATURES_ARRAY
	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);

	uint32_t value32 = 0;
	int64_t ops_per_sec;

	BENCH_PREPARE_FORCE(round)
	for (int k = 0; k < NUM_K; k++) {
		BENCH_START_FORCE(round)
		for (int j = 0; j < NUM_BENCH_FEATURE; j++) {
			pipe_mgr_begin_batch(sess_hdl);
			for (int i = 0; i < NUM_FEATURES; i++) {
				features[i]->write_fn(sess_hdl, PIPES[0], j % MAX_FLOWS, &value32);
			}
			pipe_mgr_end_batch(sess_hdl, true);
		}
		BENCH_END_FORCE(round, "Resetting features")
		ops_per_sec = BENCH_TO_OPS(round, NUM_BENCH_FEATURE * NUM_FEATURES);
		INFO_LOG("Feature Op/s: %ld", ops_per_sec);
		fflush(stdout);
	}
	switch_disconnect(sess_hdl);
	return NULL;
}


void *bench_classification_remove_with_zero(__attribute__((unused)) void *arg) {
	STATIC_FEATURES_ARRAY
	uint32_t value32 = 0;
	five_tuple_t five_tuple = {
			.h = {
					.l3_proto = L3_IPV4,
					.l4_proto = L4_TCP,
					.dst_port = 1,
					.src_port = 1,
					.dst_addr.ipv4 = 1,
			},
			.dev_port = 1,
			.ingress_ts = 1,
			.session_id_egress_offset = 0,
	};
	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);

	int64_t ops_per_sec;
	BENCH_PREPARE_FORCE(round)
	uint32_t count;

#define num_ops (MIN(MAX_FLOWS, MAX_IP4_FLOWS) - 8192 - 4098)

	ERROR_LOG("Num ops: %d", num_ops)
	p4_pd_entry_hdl_t entry_hdls[num_ops];

	for (int k = 0; k < NUM_K; k++) {
		for (uint32_t i = 0; i < num_ops; i++) {
			five_tuple.h.src_addr.ipv4 = i;
			pipe_mgr_begin_batch(sess_hdl);
			if (switch_add_classification_entry(&five_tuple, sess_hdl, i, &entry_hdls[i]) == ERR_SWITCH) {
				pipe_mgr_end_batch(sess_hdl, true);
				p4_pd_marina_data_plane_classification_ipv4_get_entry_count(sess_hdl, ALL_PIPES, &count);
				ERROR_LOG("Number of entries: %u (%u)", count, i)
				return NULL;
			}
			pipe_mgr_end_batch(sess_hdl, true);
		}


		BENCH_START_FORCE(round)
		for (uint32_t i = 0; i < num_ops; i++) {
			five_tuple.h.src_addr.ipv4 = i;
			pipe_mgr_begin_batch(sess_hdl);
			switch_remove_classification_entry(sess_hdl, &five_tuple, entry_hdls[i]);
			for (int j = 0; j < NUM_FEATURES; j++) {
				features[j]->write_fn(sess_hdl, PIPES[0], j % MAX_FLOWS, &value32);
			}
			pipe_mgr_end_batch(sess_hdl, true);
		}
		BENCH_END_FORCE(round, "Removing took")
		ops_per_sec = BENCH_TO_OPS(round, num_ops);
		INFO_LOG("Remove Zero Op/s: %ld", ops_per_sec);
		fflush(stdout);
	}

	switch_disconnect(sess_hdl);
	return NULL;
}

/*
 * TODO: measure feature reading
 * TODO: measure bloom filter performance
 * TODO: check independent threads
 */

#define THREAD_CREATE_OR_EXIT(st, fn)                                                  \
    DEBUG_LOG("starting thread " #fn)                                                  \
    pthread_t st;                                                                      \
    if (pthread_create(&(st), NULL, fn, NULL) != 0) {                                  \
        DEBUG_LOG("Error starting thread " #fn "errno %d %s", errno, strerror(errno))  \
        exit(1);                                                                       \
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

	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);
	prepare_dataplane(sess_hdl);
	prepare_features(sess_hdl);

	/*INFO_LOG("Test feature reading")
	THREAD_CREATE_OR_EXIT(bench_feature_reader1, bench_feature_reader)
	pthread_join(bench_feature_reader1, NULL);

	INFO_LOG("Test feature export data copy")
	THREAD_CREATE_OR_EXIT(bench_feature_export_data_copy1, bench_feature_export_data_copy)
	pthread_join(bench_feature_export_data_copy1, NULL);*/

	INFO_LOG("Test feature export to file")
	THREAD_CREATE_OR_EXIT(bench_feature_export_to_file1, bench_feature_export_to_file)
	pthread_join(bench_feature_export_to_file1, NULL);

//	INFO_LOG("Test insert/remove without congestion")
//	THREAD_CREATE_OR_EXIT(bench_classification_thread1, bench_classification_insert_remove)
//	pthread_join(bench_classification_thread1, NULL);
//
//	INFO_LOG("Test insert/remove with congestion")
//	THREAD_CREATE_OR_EXIT(background_feature_thread1, background_feature_reader)
//	sleep(1);
//	THREAD_CREATE_OR_EXIT(bench_classification_thread2, bench_classification_insert_remove)
//	pthread_join(bench_classification_thread2, NULL);
//	run_feature_reader = false;
//	pthread_join(background_feature_thread1, NULL);

//	THREAD_CREATE_OR_EXIT(bloom_filter_thread, bench_bloom_filter)
//	pthread_join(bloom_filter_thread, NULL);
//
//	THREAD_CREATE_OR_EXIT(feature_reset_thread, bench_feature_reset)
//	pthread_join(feature_reset_thread, NULL);

//	THREAD_CREATE_OR_EXIT(remove_reset_thread, bench_classification_remove_with_zero)
//	pthread_join(remove_reset_thread, NULL);

	INFO_LOG("Done")
	fflush(stdout);
}