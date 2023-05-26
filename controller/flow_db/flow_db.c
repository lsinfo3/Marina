#include "flow_db.h"

#include "../dataplane/prepare_dataplane.h"
#include "../datastructures/heap.h"
#include "../datastructures/flow_hashtable.h"
#include "../datastructures/session_hashtable.h"
#include "../datastructures/bloom_filter.h"


heapqueue_t flow_id_heap[NUM_PIPES];
heapqueue_t session_id_heap[NUM_PIPES];
pthread_spinlock_t uuid_mapping_lock;
char uuid_mapping[FEATURE_SIZE][UUID_STR_LEN];

GLOBAL_STAT_IMPL(flowdb)

/*
 * Return a uuid as lower case 37 character (36+'\0') string
 */
static void mk_uuid(char *uuid) {
	uuid_t binuuid;
	uuid_generate_random(binuuid);
	pthread_spin_lock(&uuid_mapping_lock);
	uuid_unparse_lower(binuuid, uuid);
	pthread_spin_unlock(&uuid_mapping_lock);
}

/*
 * Allocate flow id and uuid, add to flow hashtable, send insert command to
 * data plane thread
 */
static void add_flow(five_tuple_t five_tuple) {
	int16_t egress_port = DEVPORT_MAPPING[five_tuple.dev_port];
	if (egress_port == -1) {
		return;
	}

	int32_t flow_id = heap_pop(&flow_id_heap[dev_port_to_pipe(five_tuple.dev_port)]);

	if (flow_id == 0) {
		DEBUG_LOG("No more flow ids!!!");
		return;
	}

	five_tuple.session_id_egress_offset = dev_port_to_pipe(egress_port) - dev_port_to_pipe(five_tuple.dev_port);

	if (flow_hashtable_add(&five_tuple, flow_id) != 0) {
		// Flow is already in db
		heap_push(&flow_id_heap[dev_port_to_pipe(five_tuple.dev_port)], flow_id);
		return;
	}

	int32_t session_id;
	if (session_hashtable_get(&five_tuple, &session_id) != 0) {
		// session id does not exist until now => get new
		session_id = heap_pop(&session_id_heap[dev_port_to_pipe(five_tuple.dev_port)]);
		mk_uuid(uuid_mapping[session_id]);
		if (session_id == 0) {
			DEBUG_LOG("No more session ids!!!");
			return;
		}
	}
	session_hashtable_add_or_increase_flow_count(&five_tuple, session_id);

	dataplane_command_t command = {
			.args.match_table_cmd = {
					.five_tuple = five_tuple,
					.flow_id = flow_id,
					.session_id = session_id
			},
			.command = DP_ADD_FLOW,
	};

	if (dataplane_ringbuffer_put(&command) != 0) {
		// Inserting was not successful
		heap_push(&flow_id_heap[dev_port_to_pipe(five_tuple.dev_port)], flow_id);
		flow_hashtable_remove_by_value(flow_id, NULL);
	}
	else {
//		set_new_flow_initial_timeout(flow_id);
	}
}

static void clear_registers(const five_tuple_t five_tuple, int32_t session_id) {
	dataplane_command_t command = {
			.args.match_table_cmd = {
					.five_tuple = five_tuple,
					.session_id = session_id,
			},
			.command = DP_CLEAR_REGISTERS,
	};
	dataplane_ringbuffer_put(&command);
}

static void session_cleanup_after_flow_remove(const five_tuple_t five_tuple) {
	int32_t session_id;
	if (session_hashtable_remove_or_decrease_flow_count(&five_tuple, &session_id) == 0) {
		// return session_id for reuse
		heap_push(&session_id_heap[dev_port_to_pipe(five_tuple.dev_port)], session_id);
		clear_registers(five_tuple, session_id);
	}
}

/*
 * Free flow id and remove flow from hashtable, send remove command to
 * data plane thread
 */
static void remove_flow_by_five_tuple(const five_tuple_t five_tuple) {
	int32_t flow_id;
	if (flow_hashtable_remove(&five_tuple, &flow_id) != 0) {
		// Flow is not in hashtable anymore
		return;
	};
	heap_push(&flow_id_heap[dev_port_to_pipe(five_tuple.dev_port)], flow_id);

	dataplane_command_t command = {
			.args.match_table_cmd = {
					.five_tuple = five_tuple,
					.flow_id = flow_id,
			},
			.command = DP_REMOVE_FLOW,
	};
	dataplane_ringbuffer_put(&command);

	session_cleanup_after_flow_remove(five_tuple);
}

static void remove_flow_by_flow_id(const int32_t flow_id) {
	five_tuple_t five_tuple;
	if (flow_hashtable_remove_by_value(flow_id, &five_tuple) != 0) {
		ERROR_LOG("Tried to remove unknown flow %d from hashtable by flow_id", flow_id)
		return;
	}
	heap_push(&flow_id_heap[dev_port_to_pipe(five_tuple.dev_port)], flow_id);

	dataplane_command_t command = {
			.args.match_table_cmd = {
					.five_tuple = five_tuple,
					.flow_id = flow_id,
			},
			.command = DP_REMOVE_FLOW,
	};
	dataplane_ringbuffer_ensure_put(&command);

	session_cleanup_after_flow_remove(five_tuple);
}

_Noreturn void *flowdb_thread(void *arg __attribute__((unused))) {
	DEBUG_LOG("Starting flowdb thread")
	// Fill all flow id heaps with all available ids
	// We use a heap to keep track of all free ids and to assign
	// the lowest id possible to reduce the number of chunks that have to be
	// read from the data plane
	for (int pipe = 0; pipe < NUM_PIPES; pipe++) {
		for (int index = 0; index < HEAPSIZE; index++) {
			// globally unique flow ids. So we can see just from the flow id, on which pipe the the flow resides.
			// Also, this matches the array format when reading registers from the dp

			// TODO: maybe remove
			int flow_id = (index + 1) * 4 + pipe;
			flow_id_heap[pipe].data[index] = flow_id;

			int session_id = (index + 1) * 4 + pipe;
			session_id_heap[pipe].data[index] = session_id;
		}
		DEBUG_LOG("Building heap %d", pipe)
		heap_build(&flow_id_heap[pipe], HEAPSIZE);
		heap_build(&session_id_heap[pipe], HEAPSIZE);
	}

	DEBUG_LOG("Preparing UUID map")
	pthread_spin_lock(&uuid_mapping_lock);
	memset(uuid_mapping, 0, FEATURE_SIZE * UUID_STR_LEN * sizeof(char));
	pthread_spin_unlock(&uuid_mapping_lock);

	packet_item_t command;

	DEBUG_LOG("Waiting for new flows...")
	// Read commands from the packet ringbuffer which is filled by the packet capture thread
	// and execute them
	while (1) {
		while (packet_ringbuffer_empty());
		while (packet_ringbuffer_get(&command) == 0) {
			VERBOSE_LOG("New command for flow db")

			INC_STAT(flowdb)

			switch (command.command) {
				case PKT_ADD_FLOW:
					add_flow(command.args.five_tuple);
					break;
				case PKT_REMOVE_FLOW:
					remove_flow_by_five_tuple(command.args.five_tuple);
					break;
				case PKT_ADD_BLOOM_FILTER_IRRELEVANT:
					bloom_filter_set_irrelevant(&command.args.five_tuple);
					break;
				case PKT_FIX_BLOOM_FILTER_RELEVANT:
					bloom_filter_set_relevant(&command.args.five_tuple);
					break;
				case PKT_TIMEOUT_FLOW:
					remove_flow_by_flow_id(command.args.flow_id);
				default:
					continue;
			}
		}
	}
}
