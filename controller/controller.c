#include "controller.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "datastructures/packet_ringbuffer.h"
#include "datastructures/dataplane_ringbuffer.h"
#include "datastructures/dns_ringbuffer.h"
#include "datastructures/flow_hashtable.h"
#include "datastructures/session_hashtable.h"
#include "datastructures/bloom_filter.h"

#include "logging/statistics.h"

#include "dataplane/dataplane.h"
#include "packet_handler/packet_handler.h"
#include "flow_db/flow_db.h"
#include "export/export_features.h"
#include "dns_db/dns_db.h"
#include "runtime_config/runtime_config.h"

#define THREAD_CREATE_OR_EXIT(st, fn)                                                  \
    DEBUG_LOG("starting thread " #fn)                                                  \
    pthread_t st;                                                                      \
    if (pthread_create(&(st), NULL, fn, NULL) != 0) {                                  \
        DEBUG_LOG("Error starting thread " #fn "errno %d %s", errno, strerror(errno))  \
        exit(1);                                                                       \
    }

/*
 * Prepares, starts and joins all threads
 */
void low_level_controller() {
	load_runtime_config();

	DEBUG_LOG("Initializing global locks")
	pthread_spin_init(&uuid_mapping_lock, 0);

	DEBUG_LOG("Initializing data structures")
	packet_ringbuffer_init();
	dataplane_ringbuffer_init();
	dns_ringbuffer_init();
	flow_hashtable_init();
	session_hashtable_init();
	dns_hashtable_init();
	bloom_filter_init();

	DEBUG_LOG("Starting threads")
	THREAD_CREATE_OR_EXIT(exporter_thread_struct, feature_export_thread)
	THREAD_CREATE_OR_EXIT(dataplane_thread_struct, dataplane_thread)
	THREAD_CREATE_OR_EXIT(flowdb_thread_struct, flowdb_thread)
	THREAD_CREATE_OR_EXIT(packet_thread_struct, packet_thread)
	THREAD_CREATE_OR_EXIT(bloom_filter_thread_struct, bloom_filter_maintenance)
#ifdef USE_DNS_DB
	THREAD_CREATE_OR_EXIT(dns_db_thread_struct, dns_db_thread)
#endif
#ifdef SHOW_STATISTICS
	THREAD_CREATE_OR_EXIT(statistics_thread_struct, statistics_thread)
#endif

	pthread_join(exporter_thread_struct, NULL);
	pthread_join(dataplane_thread_struct, NULL);
	pthread_join(flowdb_thread_struct, NULL);
	pthread_join(packet_thread_struct, NULL);
	pthread_join(bloom_filter_thread_struct, NULL);
#ifdef USE_DNS_DB
	pthread_join(dns_db_thread_struct, NULL);
#endif
#ifdef SHOW_STATISTICS
	pthread_join(statistics_thread_struct, NULL);
#endif
}
