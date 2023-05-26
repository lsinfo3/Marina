/*
 * Feature aggregation for packet sizes
 * Uses ipv4/ipv6 length fields
 * This means: ipv4 does contain header, ipv6 does not!!
 * Therefore, use a lookup table for the header length of packet (including Layer 3 and 4)
 * and subtract it from the header length field
 *
 * Registers:
 * - byte_reg, 32 bit unsigned: contains accumulated packet sizes
 * - byte_square_low_reg, 32 bit unsigned: contains lower 32 bits of accumulated squared packet sizes
 * - byte_square_high_reg: like above, but higher 32 bits
 *
 * control flow:
 * - get header length from matching table
 * - subtract header size from packet length
 * - add packet length to byte_reg
 * - square packet length
 * - add lower 32 bits to byte_square_low_reg, return value from before update
 * - match old and new lower 32 bits on add_bytes_square_high_table
 *   - Contains a list of ternary entries which detect overflows in the previous addition
 *   - on a hit, add 1
 *   - on a miss, do nothing
 */

header_type meta_alu_size_temp_t {
    fields {
        header_length: 8;

        packet_size: 16;
        size_log: 8;
        size_log_square: 8;
        size_log_cube: 16;
    }
}
metadata meta_alu_size_temp_t alu_size_m;

register size_log_reg {
    width: 32;
    instance_count: MAX_FLOWS;
}

register size_log_square_reg {
    width: 32;
    instance_count: MAX_FLOWS;
}

register size_log_cube_reg {
	width: 32;
	instance_count: MAX_FLOWS;
}

/*
 * Retrieve total header size of this packet
 */
action header_length_hit(header_length) {
    modify_field(alu_size_m.header_length, header_length);
}
action header_length_miss() {}
table header_length_table {
    reads {
#ifdef ENABLE_IP4
        ipv4: valid;
        ipv4.ihl: ternary;
#endif
#ifdef ENABLE_IP6
        ipv6: valid;
#endif
#ifdef ENABLE_TCP
        tcp: valid;
        tcp.dataOffset: ternary;
#endif
#ifdef ENABLE_UDP
        udp: valid;
#endif
    }
    actions {
        header_length_hit;
        header_length_miss;
    }
    default_action: header_length_miss();
    size: HEADER_LENGTH_TABLE_SIZE;
}

/*
 * Subtract header length from packet length.
 * This cannot be performed in the previous action due to limitations of the Tofino platform,
 * subtract cannot work with action data, just with header or metadata fields
 */
action subtract_header_length_action() {
    subtract(alu_size_m.packet_size, l3_lookup.len, alu_size_m.header_length);
}
ACTION2TABLE(subtract_header_length)

action compute_size_log_action(log, log_square, log_cube) {
    modify_field(alu_size_m.size_log, log);
    modify_field(alu_size_m.size_log_square, log_square);
    modify_field(alu_size_m.size_log_cube, log_cube);
}
table compute_size_log_table {
    reads {
            alu_size_m.packet_size: lpm;
    }
    actions {
        compute_size_log_action;
    }
    size: BYTE_LOG_TABLE_SIZE;
}

/*
 * Accumulate packet size
 */
blackbox stateful_alu add_size_log_alu {
    reg: size_log_reg;
    update_lo_1_value: register_lo + alu_size_m.size_log;
}
BLACKBOX2TABLE(add_size_log)


blackbox stateful_alu add_size_log_square_alu {
    reg: size_log_square_reg;
    update_lo_1_value: register_lo + alu_size_m.size_log_square;
}
BLACKBOX2TABLE(add_size_log_square)

blackbox stateful_alu add_size_log_cube_alu {
	reg: size_log_cube_reg;
	update_lo_1_value: register_lo + alu_size_m.size_log_cube;
}
BLACKBOX2TABLE(add_size_log_cube)

control size_features {
    apply(add_size_log_table);
    apply(add_size_log_square_table);
    apply(add_size_log_cube_table);
}
