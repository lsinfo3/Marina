#ifndef LOW_LEVEL_LIB_FLOW_DB_H
#define LOW_LEVEL_LIB_FLOW_DB_H

#include "../config.h"

#include <uuid/uuid.h>


#include "../datastructures/packet_ringbuffer.h"
#include "../datastructures/dataplane_ringbuffer.h"
#include "../logging/statistics.h"

extern pthread_spinlock_t uuid_mapping_lock;
extern char uuid_mapping[FEATURE_SIZE][UUID_STR_LEN];

GLOBAL_STAT_HDR(flowdb)

_Noreturn void *flowdb_thread(void *args);

#endif //LOW_LEVEL_LIB_FLOW_DB_H
