#include "statistics.h"

#include <unistd.h>
#include <pcap.h>

#include "../datastructures/packet_ringbuffer.h"
#include "../datastructures/dataplane_ringbuffer.h"
#include "../datastructures/dns_ringbuffer.h"
#include "../packet_handler/packet_handler.h"
#include "../flow_db/flow_db.h"
#include "../dns_db/dns_db.h"
#include "../dataplane/dataplane.h"

#ifdef SHOW_STATISTICS
_Noreturn void *statistics_thread(__attribute__((unused)) void *arg) {
	PREP_READ_STAT(packet_ringbuffer)
	PREP_READ_STAT(dns_ringbuffer)
	PREP_READ_STAT(dp_ringbuffer)

	PREP_READ_STAT(packet_handler)
	PREP_READ_STAT(dnsdb)
	PREP_READ_STAT(flowdb)
	PREP_READ_STAT(dataplane_commands)

	PREP_READ_STAT(dataplane_add)
	PREP_READ_STAT(dataplane_remove)
	PREP_READ_STAT(dataplane_reset)
	PREP_READ_STAT(dataplane_bloom_add)
	PREP_READ_STAT(dataplane_bloom_remove)


#ifdef PCAP_STATISTICS
	unsigned int ps_recv_last = 0, ps_drop_last = 0, ps_ifdrop_last = 0;
#endif
	while (1) {
		usleep(1000 * 1000);
		READ_STAT(dns_ringbuffer, "DNS ringbuffer drop stats")
		READ_STAT(packet_ringbuffer, "packet ringbuffer drop stats")
		READ_STAT(dp_ringbuffer, "data plane ringbuffer drop stats")

		READ_STAT(packet_handler, "packet handler stats")
		READ_STAT(dnsdb, "DNSdb stats")
		READ_STAT(flowdb, "Flowdb stats")
		READ_STAT(dataplane_commands, "data plane stats")

		READ_STAT(dataplane_add, "DP add")
		READ_STAT(dataplane_remove, "DP remove")
		READ_STAT(dataplane_reset, "DP reset")
		READ_STAT(dataplane_bloom_add, "DP bloom set")
		READ_STAT(dataplane_bloom_remove, "DP bloom clear")

#ifdef PCAP_STATISTICS
		struct pcap_stat pcap_statistics;
		pcap_stats(pcap_descr, &pcap_statistics);
		INFO_LOG("Pcap statistics: received: %d (%d/s), dropped by kernel: %d (%d/s), dropped by NIC: %d (%d/s)",
				 pcap_statistics.ps_recv, pcap_statistics.ps_recv - ps_recv_last,
				 pcap_statistics.ps_drop, pcap_statistics.ps_drop - ps_drop_last,
				 pcap_statistics.ps_ifdrop, pcap_statistics.ps_ifdrop - ps_ifdrop_last);
		ps_recv_last = pcap_statistics.ps_recv;
		ps_drop_last = pcap_statistics.ps_drop;
		ps_ifdrop_last = pcap_statistics.ps_ifdrop;
#endif
		fflush(stdout);
	}
}
#endif