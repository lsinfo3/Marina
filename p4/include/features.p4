/*
 * This file contains the packet count feature and also imports iat and byte features
 */


#include "features/size.p4"
#include "features/iat.p4"
//#include "features/tcp_retransmission.p4"

/*
 * PACKET COUNTER
 */

register pkt_count_reg {
    width: 32;
    instance_count: MAX_FLOWS;
}

blackbox stateful_alu pkt_count_alu {
    reg: pkt_count_reg;
    update_lo_1_value: register_lo + 1;
}
BLACKBOX2ACTION(pkt_count)
@pragma stage 0
ACTION2TABLE(pkt_count)


//register tcp_flags_reg {
//	width: 8;
//  instance_count: MAX_FLOWS;
//}

//blackbox stateful_alu tcp_flags_alu {
//	reg: tcp_flags_reg;
//	update_lo_1_value: register_lo | l4_lookup.tcp_flags;
//}
//BLACKBOX2TABLE(tcp_flags)

/*
 * CONTROLS
 */
control features_ingress {
//	tcp_retransmission_features();
//	size_features();

	if (valid(tcp)) {
//		apply(check_seq_num_table);
//		apply(add_retransmission_table);
	}
	apply(split_timestamp_into_high_and_low_table);
	apply(iat_read_update_last_timestamp_table);
	if (alu_iat_m.last_timestamp_low != 0) {
		apply(calculate_timestamp_diff_table);
		apply(iat_compute_log_table);
//		apply(add_iat_log_table);
////        apply(iat_log_max_table);
////        apply(iat_log_min_table);
//        apply(add_iat_log_square_table);
//        apply(add_iat_log_cube_table);
	}
}

control features_egress {
		apply(pkt_count_table);
		if (alu_iat_m.last_timestamp_low != 0) {
			apply(add_iat_log_table);
			apply(add_iat_log_square_table);
    	    apply(add_iat_log_cube_table);
		}
		// if (valid(tcp)) {
		// 	apply(copy_seqno_table);
		// 	apply(check_seq_num_table);
		// 	apply(add_retransmission_table);
		// }
		size_features();
//		iat_features();
}
