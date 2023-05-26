/*
 * Bloom filter to detect known/unknown UDP flows
 * - Entries are set and deleted by the controller
 * - Four 1 bit registers, each BLOOMFILTER_WIDTH long
 * - Four hash functions: crc32, crc32c, crc32d, crc32q
 *   Output is capped to BLOOMFILTER_BITS
 * - Metadata in_{1,2,3,4} holds bool if entry was found by hash or not
 */

#ifdef ENABLE_UDP

header_type bloom_meta_t {
    fields {
        not_in_1: 1;
        not_in_2: 1;
        not_in_3: 1;
        not_in_4: 1;
        ipv6_hash1: 32;
        ipv6_hash2: 32;
    }
}
metadata bloom_meta_t bloom_m;

field_list ipv6_bloom_hash_list1 {
	ipv6.srcAddr;
}
field_list_calculation ipv6_bloom_hash1 {
    input { ipv6_bloom_hash_list1; }
    algorithm: crc_32;
    output_width: BLOOMFILTER_BITS;
}
action calc_ipv6_bloom_hash1_action() {
    modify_field_with_hash_based_offset(bloom_m.ipv6_hash1, 0, ipv6_bloom_hash1, 0x100000000);
}
ACTION2TABLE(calc_ipv6_bloom_hash1)

field_list ipv6_bloom_hash_list2 {
	ipv6.dstAddr;
}
field_list_calculation ipv6_bloom_hash2 {
    input { ipv6_bloom_hash_list2; }
    algorithm: crc_32;
    output_width: BLOOMFILTER_BITS;
}
action calc_ipv6_bloom_hash2_action() {
    modify_field_with_hash_based_offset(bloom_m.ipv6_hash2, 0, ipv6_bloom_hash2, 0x100000000);
}
ACTION2TABLE(calc_ipv6_bloom_hash2)

/*
 * We can list ipv4 and ipv6 here, the hash function will ignore fields
 * in an invalid header
 */
field_list bloom_hash_list {
#ifdef ENABLE_IP4
    ipv4.srcAddr;
    ipv4.dstAddr;
#endif
#ifdef ENABLE_IP6
    bloom_m.ipv6_hash1;
    bloom_m.ipv6_hash2;
#endif
    udp.srcPort;
    udp.dstPort;
}

#define BLOOM_FILTER(NUM, HASH_FUNC)                                    \
register bloom_filter_##NUM {                                           \
    width: 1;                                                           \
    instance_count: BLOOMFILTER_WIDTH;                                  \
}                                                                       \
blackbox stateful_alu check_bloom_##NUM {                               \
    reg: bloom_filter_##NUM;                                            \
    update_lo_1_value: clr_bitc;                                        \
    output_value: alu_lo;                                               \
    output_dst: bloom_m.not_in_##NUM;                                   \
}                                                                       \
field_list_calculation hash_crc##NUM {                                  \
    input { bloom_hash_list; }                                          \
    algorithm: HASH_FUNC;                                               \
    output_width: BLOOMFILTER_BITS;                                     \
}                                                                       \
action bloom_filter_##NUM##_action() {                                  \
    check_bloom_##NUM.execute_stateful_alu_from_hash(hash_crc##NUM);    \
}                                                                       \
ACTION2TABLE(bloom_filter_##NUM)


BLOOM_FILTER(1, crc_32)
BLOOM_FILTER(2, crc_32c)
BLOOM_FILTER(3, crc_32d)
BLOOM_FILTER(4, crc_32q)


control check_bloom {
    apply(bloom_filter_1_table);
    apply(bloom_filter_2_table);
    apply(bloom_filter_3_table);
    apply(bloom_filter_4_table);
}

#endif //ENABLE_UDP
