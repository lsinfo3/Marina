/*
 * Feature aggregation for inter arrival times
 * Uses ig_intr_md.ingress_mac_tstamp for timestamps in nanoseconds
 * cut off at 32 bits -> at maximum 4.3 seconds
 * controller handles IATs greater than that
 *
 * Registers:
 * - iat_reg, 32 bit unsigned: contains accumulated IATs
 * - iat_last_timestamp_reg, 32 bit unsigned: contains timestamp of previous packet
 * - iat_square_low_reg, 32 bit unsigned: contains lower 32 bits of accumulated squared IATs
 * - iat_square_high_reg: like above, but higher 32 bits
 *
 * control flow:
 * - Get last timestamp and save new one
 * - If last timestamp was 0, this is the first packet in this flow -> ignore
 * - compute difference
 * - add difference to register
 * - shift difference by 2 to the right, reducing resolution (currently required for control plane)
 * - square difference
 * - add to iat_square_low_reg, return value from before update
 * - match old and new lower 32 bits on add_iat_square_high_table
 *   - Contains a list of ternary entries which detect overflows in the previous addition
 *   - on a hit, add higher 32 bits + 1
 *   - on a miss, add just the higher 32 bits
 */

header_type meta_alu_iat_temp_t {
    fields {
        timestamp_low: 32;
        last_timestamp_low: 32;

        iat: 32;
        iat_log: 8;
        iat_log_square: 16;
        iat_log_cube: 16;
    }
}
metadata meta_alu_iat_temp_t alu_iat_m;

register iat_last_timestamp_reg {
    width: 32;
    instance_count: MAX_FLOWS;
}

register iat_log_reg {
    width: 32;
    instance_count: MAX_FLOWS;
}

//register iat_log_max_reg {
//    width: 8;
//    instance_count: MAX_FLOWS;
//}
//
//register iat_log_min_reg {
//    width: 8;
//    instance_count: MAX_FLOWS;
//}

register iat_log_square_reg {
    width: 32;
    instance_count: MAX_FLOWS;
}

register iat_log_cube_reg {
	width: 32;
	instance_count: MAX_FLOWS;
}

action split_timestamp_into_high_and_low_action() {
    modify_field(alu_iat_m.timestamp_low, ig_intr_md.ingress_mac_tstamp);
}
ACTION2TABLE(split_timestamp_into_high_and_low)

/*
 * Write new timestamp, retrieve old one
 */
blackbox stateful_alu iat_read_update_last_timestamp_alu {
    reg: iat_last_timestamp_reg;
    update_lo_1_value: alu_iat_m.timestamp_low;
    output_value: register_lo;
    output_dst: alu_iat_m.last_timestamp_low;
}
BLACKBOX2ACTION(iat_read_update_last_timestamp)
//@pragma stage 1
ACTION2TABLE(iat_read_update_last_timestamp)

/*
 * Compute iat
 */
action calculate_timestamp_diff_action() {
    subtract(alu_iat_m.iat, alu_iat_m.timestamp_low, alu_iat_m.last_timestamp_low);
}
//@pragma stage 2
ACTION2TABLE(calculate_timestamp_diff)

action iat_compute_log_action(log, log_square, log_cube) {
    modify_field(alu_iat_m.iat_log, log);
    modify_field(alu_iat_m.iat_log_square, log_square);
    modify_field(alu_iat_m.iat_log_cube, log_cube);
}

//@pragma stage 4
table iat_compute_log_table {
    reads {
        alu_iat_m.iat: lpm;
    }
    actions {
        iat_compute_log_action;
    }
    size: IAT_LOG_TABLE_SIZE;
}

blackbox stateful_alu add_iat_log_alu {
    reg: iat_log_reg;
    update_lo_1_value: register_lo + alu_iat_m.iat_log;
}
BLACKBOX2ACTION(add_iat_log)
@pragma stage 1
ACTION2TABLE(add_iat_log)

//blackbox stateful_alu iat_log_max_alu {
//    reg: iat_log_max_reg;
//    condition_lo: register_lo < alu_iat_m.iat_log;
//
//    update_lo_1_predicate: condition_lo;
//    update_lo_1_value: alu_iat_m.iat_log;
//}
//BLACKBOX2TABLE(iat_log_max)
//
//blackbox stateful_alu iat_log_min_alu {
//    reg: iat_log_min_reg;
//    condition_lo: register_lo > alu_iat_m.iat_log;
//
//    update_lo_1_predicate: condition_lo;
//    update_lo_1_value: alu_iat_m.iat_log;
//}
//BLACKBOX2TABLE(iat_log_min)

blackbox stateful_alu add_iat_log_square_alu {
    reg: iat_log_square_reg;
    update_lo_1_value: register_lo + alu_iat_m.iat_log_square;
}
BLACKBOX2ACTION(add_iat_log_square)
@pragma stage 2
ACTION2TABLE(add_iat_log_square)

blackbox stateful_alu add_iat_log_cube_alu {
	reg: iat_log_cube_reg;
	update_lo_1_value: register_lo + alu_iat_m.iat_log_cube;
}
BLACKBOX2ACTION(add_iat_log_cube)
@pragma stage 3
ACTION2TABLE(add_iat_log_cube)

control iat_features {
	apply(iat_read_update_last_timestamp_table);
    // Should we have a last_timestamp of zero, then we never had a timestamp before
    // and therefore cannot compute an iat
    if (alu_iat_m.last_timestamp_low != 0) {
        apply(calculate_timestamp_diff_table);
        apply(iat_compute_log_table);
        apply(add_iat_log_table);
////        apply(iat_log_max_table);
////        apply(iat_log_min_table);
//        apply(add_iat_log_square_table);
//        apply(add_iat_log_cube_table);
    }
}

