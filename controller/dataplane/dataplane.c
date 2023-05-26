#include "dataplane.h"

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include "../datastructures/dataplane_ringbuffer.h"
#include "dp_api.h"
#include "features.h"
#include "prepare_dataplane.h"

GLOBAL_STAT_IMPL(dataplane_commands)
GLOBAL_STAT_IMPL(dataplane_add)
GLOBAL_STAT_IMPL(dataplane_remove)
GLOBAL_STAT_IMPL(dataplane_reset)
GLOBAL_STAT_IMPL(dataplane_bloom_add)
GLOBAL_STAT_IMPL(dataplane_bloom_remove)

/*
 * Cache for classification table entry handles
 * Saves one controller <-> data plane round trip compared to delete operations on the match_spec
 */
static p4_pd_entry_hdl_t entry_hdls[FEATURE_SIZE];


static inline void
clear_registers(const p4_pd_sess_hdl_t sess_hdl, const int32_t session_id, const five_tuple_t five_tuple) {
	STATIC_FEATURES_ARRAY
	for (int i = 0; i < NUM_FEATURES; i++) {
		features[i]->values[session_id + features[i]->on_egress * five_tuple.session_id_egress_offset] = 0;
	}
	uint32_t zero32 = 0;
	register_last_timestamp.write_fn(
			sess_hdl,
			PIPES[flow_id_to_pipe(session_id)],
			flow_id_to_register_index(session_id),
			&zero32
	);
}

_Noreturn void *dataplane_thread(void *args __attribute__((unused))) {
	DEBUG_LOG("Starting dataplane thread")
	pipe_sess_hdl_t sess_hdl;
	switch_connect(&sess_hdl);
	int status = 0;

	dataplane_command_t command;

	prepare_dataplane(sess_hdl);
	prepare_features(sess_hdl);

	DEBUG_LOG("Waiting for dataplane commands")
	while (1) {
		while (dataplane_ringbuffer_empty()) {}

//		VERBOSE_LOG("New dataplane command")
		while (dataplane_ringbuffer_get(&command) != -1) {
			INC_STAT(dataplane_commands)
			switch (command.command) {
				case DP_ADD_FLOW:
					VERBOSE_LOG("Dataplane adding entry")
					INC_STAT(dataplane_add)
					status = switch_add_classification_entry(
							&command.args.match_table_cmd.five_tuple,
							sess_hdl, flow_id_to_register_index(command.args.match_table_cmd.session_id),
							&entry_hdls[command.args.match_table_cmd.flow_id]
					);
					break;
				case DP_REMOVE_FLOW:
					VERBOSE_LOG("Dataplane removing entry")
					INC_STAT(dataplane_remove)
					status = switch_remove_classification_entry(
							sess_hdl, &command.args.match_table_cmd.five_tuple,
							entry_hdls[command.args.match_table_cmd.flow_id]
					);
					break;
				case DP_CLEAR_REGISTERS:
					VERBOSE_LOG("Dataplane resetting entry")
					INC_STAT(dataplane_reset)
					clear_registers(sess_hdl, command.args.match_table_cmd.session_id, command.args.match_table_cmd.five_tuple);
					break;
				case DP_ADD_BLOOM_FILTER:
					VERBOSE_LOG("Dataplane setting bloom filter")
					INC_STAT(dataplane_bloom_add)
					status = switch_write_bloom_filter(
							sess_hdl,
							command.args.bloom_filter_cmd.pipe_id,
							command.args.bloom_filter_cmd.bloom_filter_indices,
							1
					);
					break;
				case DP_REMOVE_BLOOM_FILTER:
					VERBOSE_LOG("Dataplane clearing bloom filter")
					INC_STAT(dataplane_bloom_remove)
//					status = switch_write_bloom_filter(
//							sess_hdl,
//							command.args.bloom_filter_cmd.pipe_id,
//							command.args.bloom_filter_cmd.bloom_filter_indices,
//							0
//					);
					break;
				default:
					continue;
			}
			if (status != SUCCESS) {
				printf("Error with dataplane command: %d, %s\n", status,
					   ERR_TO_STR[status]);
			}
		}
	}
}