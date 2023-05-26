/*
 * This file contains the 5-tuple -> flow_id mapping table.
 * The table is split into IPv4 and IPv6 due to address size differences.
 *
 * For IPv6, there is the option to hash the two 128 bit addresses
 * into two 32 bit values for mapping with collision possibility.
 * This has a huge size advantage, as a lot less SRAM is required.
 * WARNING: Currently no controller support
 *
 * The metadata l4_lookup.word_{1,2} contain the source and destination port
 * independently from the parser.
 */

action classification_hit_action(flow_id) {
    modify_field(m.flow_id, flow_id);
}
action classification_miss_action() {}

#ifdef ENABLE_IP4
@pragma stage 1 32768
@pragma stage 2 32768
@pragma stage 3 32768
//@pragma ways 1
//@pragma force_immediate 1
table classification_ipv4 {
    reads {
        ipv4.srcAddr: exact;
        ipv4.dstAddr: exact;
        l4_lookup.src: exact;
        l4_lookup.dst: exact;
        tcp: valid;
    }
    actions {
        classification_hit_action;
        classification_miss_action;
    }
    default_action: classification_miss_action();
    size: MAX_IP4_FLOWS;
}
#endif

#ifdef ENABLE_IP6
@pragma stage 4 13000
@pragma stage 5 13000
@pragma stage 6
//@pragma stage 7 8192
table classification_ipv6 {
    reads {
        ipv6.srcAddr: exact;
        ipv6.dstAddr: exact;
        l4_lookup.src: exact;
        l4_lookup.dst: exact;
        tcp: valid;
    }
    actions {
        classification_hit_action;
        classification_miss_action;
    }
    default_action: classification_miss_action();
    size: MAX_IP6_FLOWS;
}
#endif
