#ifndef LOW_LEVEL_LIB_DATAPLANE_RINGBUFFER_H
#define LOW_LEVEL_LIB_DATAPLANE_RINGBUFFER_H

#include "mk_ringbuffer.h"
#include "structs.h"
#include "../config.h"
#include "../logging/statistics.h"

GLOBAL_STAT_HDR(dp_ringbuffer)


typedef struct {
	union {
		struct {
			five_tuple_t five_tuple;
			int32_t flow_id;
			int32_t session_id;
		} match_table_cmd;
		struct {
			int bloom_filter_indices[NUM_BLOOMFILTERS];
			int pipe_id;
		} bloom_filter_cmd;
	} args;
	enum {
		DP_ADD_FLOW,
		DP_REMOVE_FLOW,
		DP_CLEAR_REGISTERS,
		DP_ADD_BLOOM_FILTER,
		DP_REMOVE_BLOOM_FILTER,
	} command;
} dataplane_command_t;

MK_RINGBUFFER_HEADER(dataplane, dataplane_command_t, DATAPLANE_RINGBUFFER_SIZE)

#endif //LOW_LEVEL_LIB_DATAPLANE_RINGBUFFER_H
