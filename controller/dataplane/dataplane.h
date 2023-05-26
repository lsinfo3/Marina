#ifndef LOW_LEVEL_LIB_DATAPLANE_H
#define LOW_LEVEL_LIB_DATAPLANE_H

#include <stdint.h>
#include <netinet/ip6.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofinopd/marina_data_plane/pd/pd.h>
#include "../logging/statistics.h"

GLOBAL_STAT_HDR(dataplane_commands)
GLOBAL_STAT_HDR(dataplane_add)
GLOBAL_STAT_HDR(dataplane_remove)
GLOBAL_STAT_HDR(dataplane_reset)
GLOBAL_STAT_HDR(dataplane_bloom_add)
GLOBAL_STAT_HDR(dataplane_bloom_remove)

_Noreturn void *dataplane_thread(void *args);

#endif //LOW_LEVEL_LIB_DATAPLANE_H
