#include "packet_ringbuffer.h"

GLOBAL_STAT_IMPL(packet_ringbuffer)

MK_RINGBUFFER_IMPL(packet, packet_item_t, PACKET_RINGBUFFER_SIZE, packet_ringbuffer)
