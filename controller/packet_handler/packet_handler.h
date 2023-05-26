#ifndef LOW_LEVEL_LIB_PACKET_HANDLER_H
#define LOW_LEVEL_LIB_PACKET_HANDLER_H
#include <pcap.h>
#include "../logging/statistics.h"
extern pcap_t *pcap_descr;

GLOBAL_STAT_HDR(packet_handler)


void *packet_thread(void *args);

#endif //LOW_LEVEL_LIB_PACKET_HANDLER_H
