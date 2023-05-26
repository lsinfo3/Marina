#include "dataplane_ringbuffer.h"

GLOBAL_STAT_IMPL(dp_ringbuffer)

MK_RINGBUFFER_IMPL(dataplane, dataplane_command_t, DATAPLANE_RINGBUFFER_SIZE, dp_ringbuffer)
