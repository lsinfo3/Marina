#ifndef LOW_LEVEL_LIB_PACKET_RINGBUFFER_H
#define LOW_LEVEL_LIB_PACKET_RINGBUFFER_H

#include "mk_ringbuffer.h"
#include "../config.h"
#include "structs.h"
#include "../logging/statistics.h"


typedef struct {
	union {
		five_tuple_t five_tuple;
		int32_t flow_id;
	} args;
	enum {
		PKT_ADD_FLOW,
		PKT_REMOVE_FLOW,
		PKT_ADD_BLOOM_FILTER_IRRELEVANT,
		PKT_FIX_BLOOM_FILTER_RELEVANT,
		PKT_REMOVE_BLOOM_FILTER,
		PKT_TIMEOUT_FLOW,
	} command;
} packet_item_t;

GLOBAL_STAT_HDR(packet_ringbuffer)

MK_RINGBUFFER_HEADER(packet, packet_item_t, PACKET_RINGBUFFER_SIZE)

#endif //LOW_LEVEL_LIB_PACKET_RINGBUFFER_H
