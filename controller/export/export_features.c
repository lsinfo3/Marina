#include "export_features.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>

#include "../flow_db/flow_db.h"

#ifdef DEBUG

#include <sys/time.h>

#endif

#define SWAP_TIMEVAL(a, b) {struct timespec *temp = (a); (a) = (b); (b) = temp;}
//uuid;SrcIP;DstIP;SrcPort;DstPort;isTCP;firstPacketDPTs;

typedef struct __attribute__((__packed__)) {
	char uuid[UUID_STR_LEN];
	uint8_t isTCP;
	char srcIP[INET6_ADDRSTRLEN];
	char dstIP[INET6_ADDRSTRLEN];
	uint16_t srcPort;
	uint16_t dstPort;
	uint64_t firstPacketDPTs;
	uint64_t features[NUM_FEATURES];
} export_t;

/*
 * Print the five tuple with correctly parsed ip addresses to file
 */
 void print_session_five_tuple(five_tuple_t *five_tuple, FILE *file, int32_t session_id, char *uuid, int32_t flow_count) {
	int af = five_tuple->h.l3_proto;

	char client[INET6_ADDRSTRLEN];
	if (five_tuple->sh.to_server) {
		inet_ntop(af, (void *) &five_tuple->h.src_addr, client, INET6_ADDRSTRLEN);
	}
	else {
		inet_ntop(af, (void *) &five_tuple->h.dst_addr, client, INET6_ADDRSTRLEN);
	}
	fprintf(file, "%s;%d;%lu;%d;", client, five_tuple->sh.to_server, five_tuple->ingress_ts, flow_count);
#ifdef DEBUG
	fprintf(file, "%d;%d;", session_id, five_tuple->dev_port);
#endif
}

void print_header(FILE *file, feature_register_t* features[NUM_FEATURES]) {
	// First line is timestamp
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
		printf("Error reading CLOCK_REALTIME %d %s", errno, strerror(errno));
	}

#ifdef EXPORT_NUMPY
	fwrite(&ts, sizeof(struct timespec), 1, file);
#else
	fprintf(file, "%ld.%09ld\n", ts.tv_sec, ts.tv_nsec);

	// Next line is csv header
	fprintf(file, "uuid;clientIP;toServer;firstPacketDPTs;flowCount;");
#ifdef DEBUG
	fprintf(file, "session_id;dev_port;");
#endif

	for (int i = 0; i < NUM_FEATURES; i++) {
		fprintf(file, "%s;", features[i]->name);
	}
	fprintf(file, "\n");
#endif
}

void print_flow_data(FILE *file, session_hashtable_t *hashtable, feature_register_t* features[NUM_FEATURES], char uuid_mapping_copy[FEATURE_SIZE][UUID_STR_LEN]) {
	uint32_t num_flows = 0;

	// Make a linear search over the whole hashtable entry index.
	// Not ideal, but we have no dynamic list of all available five tuples or flow ids
	for (int i = MIN_SESSION_ID; i < FEATURE_SIZE; i++) {
		int entry_index;
		if ((entry_index = hashtable->feature_index_to_entry_map[i]) == -1) {
			continue;
		}
		int32_t session_id = hashtable->entries[entry_index].value;
		five_tuple_t *five_tuple = &hashtable->entries[entry_index].key;
		int32_t flow_count = hashtable->flow_count[session_id];

#ifdef EXPORT_NUMPY
		export_t *export_entry = &export_array[num_flows];

			memcpy(&export_entry->uuid, uuid_mapping_copy[session_id], UUID_STR_LEN);
			export_entry->isTCP = five_tuple->h.l4_proto == L4_TCP;
			inet_ntop(five_tuple->h.l3_proto, (void *) &five_tuple->h.src_addr, export_entry->srcIP, INET6_ADDRSTRLEN);
			inet_ntop(five_tuple->h.l3_proto, (void *) &five_tuple->h.dst_addr, export_entry->dstIP, INET6_ADDRSTRLEN);
			export_entry->srcPort = five_tuple->h.src_port;
			export_entry->dstPort = five_tuple->h.dst_port;
			export_entry->firstPacketDPTs = five_tuple->ingress_ts;

			for (int j=0; j<NUM_FEATURES; j++) {
				export_entry->features[j] = features[j]->values[session_id + features[j]->on_egress * five_tuple->session_id_egress_offset];
			}

#else
		print_session_five_tuple(five_tuple, file, session_id, uuid_mapping_copy[session_id], flow_count);
		for (int j = 0; j < NUM_FEATURES; j++) {
			fprintf(file, "%lu;",
					features[j]->values[session_id + features[j]->on_egress * five_tuple->session_id_egress_offset]);
		}
		fprintf(file, "\n");
#endif
		num_flows++;
	}

#ifdef EXPORT_NUMPY
	fwrite(&num_flows, sizeof(num_flows), 1, file);
		fwrite(&export_array, sizeof(export_t), num_flows, file);
#else
	fprintf(file, "\n");
#endif
	fflush(file);

#ifdef DEBUG
	printf("Exported %d flows\n", num_flows);
#endif
}

FILE* open_output_file() {
	DEBUG_LOG("Opening output file")
	FILE *file;
#ifdef EXPORT_NUMPY
	if ((file = fopen(FEATURE_PIPE, "wb")) == NULL) {
#else
	if ((file = fopen(FEATURE_PIPE, "w")) == NULL) {
#endif
		int erropen = errno;
		printf("Error when opening file");
		exit(erropen);
	}
	return file;
}

_Noreturn void *feature_export_thread(void *arg __attribute__((unused))) {
	STATIC_FEATURES_ARRAY

	p4_pd_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);

	// Prepare buffers for a threadsafe copy of features and hashtable_copy
//	static uint64_t value_copies[NUM_FEATURES][FEATURE_SIZE];
	static session_hashtable_t hashtable_copy;
	static char uuid_mapping_copy[FEATURE_SIZE][UUID_STR_LEN];

#ifdef EXPORT_NUMPY
	static export_t export_array[FEATURE_SIZE];
#endif

	FILE* file = open_output_file();

	struct timespec time_a, time_b;
	struct timespec *now = &time_a, *last = &time_b;
	clock_gettime(CLOCK_MONOTONIC, last);

	while (1) {
		while (1) {
			clock_gettime(CLOCK_MONOTONIC, now);
			if (now->tv_sec > last->tv_sec && now->tv_nsec >= last->tv_nsec) {
				INFO_LOG("Reading features from dataplane")
				read_features(sess_hdl);
				SWAP_TIMEVAL(now, last)
				break;
			}
		}

		BENCH_PREPARE(export)
		BENCH_PREPARE(locked)
		BENCH_PREPARE(wait_lock)
		BENCH_PREPARE(hashtable)
		BENCH_PREPARE(uuid)

		BENCH_START(export)
		BENCH_START(locked)
		BENCH_START(wait_lock)
		// Copy features in a buffer so that we can read them without locking the dataplane thread
//		pthread_spin_lock(&dataplane_feature_lock);
		pthread_spin_lock(&uuid_mapping_lock);
		BENCH_END(wait_lock, "waiting for lock")

		BENCH_START(hashtable)
		// Copy flow hashtable into a buffer to iterate over current flows
		session_hashtable_copy(&hashtable_copy);
		BENCH_END(hashtable, "copying hashtable")

		BENCH_START(uuid)
		// Copy UUID mapping
		memcpy(uuid_mapping_copy, uuid_mapping, sizeof(char) * FEATURE_SIZE * UUID_STR_LEN);
		BENCH_END(uuid, "copying uuid")

		pthread_spin_unlock(&uuid_mapping_lock);

		BENCH_END(locked, "locked for at total of")

		print_header(file, features);

		print_flow_data(file, &hashtable_copy, features, uuid_mapping_copy);

		BENCH_END(export, "Export took")
	}
}
