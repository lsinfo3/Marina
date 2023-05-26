#ifdef __TARGET_TOFINO__
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>
#include <tofino/stateful_alu_blackbox.p4>
#else
#error This program is intended to compile for Tofino P4 architecture only
#endif

#include "config.p4"


header_type meta_t {
    fields {
#ifdef IP6_ONLY_MATCH_CRC
        ipv6_match_hash1: 32;
        ipv6_match_hash2: 32;
#endif
        flow_id: 20;
        ignore_flow: 1;
    }
}
metadata meta_t m;

action noop() {}

#include "include/macros.p4"
#include "include/parser.p4"
#include "include/classification.p4"
#include "include/features.p4"
#include "include/forward.p4"

#ifdef ENABLE_UDP
#include "include/bloomfilter.p4"
#endif

control ingress {
	forward();
	apply(header_length_table);
	apply(subtract_header_length_table);

//	apply(copy_seqno_table);
	apply(calc_ipv6_bloom_hash1_table);
	apply(calc_ipv6_bloom_hash2_table);

#ifdef ENABLE_IP4
	if (valid(ipv4)) {
		apply(classification_ipv4);
	}
#ifdef ENABLE_IP6
	else
#endif // ENABLE_IP6
#endif // ENABLE_IP4

#ifdef ENABLE_IP6
	if (valid(ipv6)) {
		apply(classification_ipv6);
	}
#endif // ENABLE_IP6

#ifdef ENABLE_UDP
	if (valid(udp) and m.flow_id == 0) {
		check_bloom();
	}
#endif

	if (m.flow_id == 0) {
		if (valid(tcp) and m.flow_id == 0 and tcp.SYN == 1) {
#ifdef ENABLE_TCP
			apply(to_controller_table);
#endif
		}
		else if (valid(udp)){
			if (valid(dns) and dns.qr == 1 and dns.opcode == 0 and dns.rcode == 0) {
#ifdef USE_DNS_DB
				apply(to_controller_table);
#endif
			}
			else if (bloom_m.not_in_1 == 1 or
				bloom_m.not_in_2 == 1 or
				bloom_m.not_in_3 == 1 or
				bloom_m.not_in_4 == 1) {
#ifdef ENABLE_UDP
				apply(to_controller_table);
#endif
			}
		}
	}
	else {
		if (valid(tcp) and tcp.FIN == 1) {
#ifdef ENABLE_TCP
			apply(to_controller_table);
#endif
		}
	}
	apply(compute_size_log_table);
	features_ingress();
}

control egress {
    if (eg_intr_md.egress_port == CPU_PORT) {
        apply(save_ingress_port_to_ethernet_src_table);
    }
    else {
		features_egress();
	}
}
