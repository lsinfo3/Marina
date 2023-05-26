header_type meta_alu_tcp_retransmission_temp_t {
	fields {
		seqno: 16;
		retransmission: 1;
	}
}
metadata meta_alu_tcp_retransmission_temp_t tcp_re_m;

register last_seq_reg {
	width: 16;
	instance_count: MAX_FLOWS;
}

register ret_count_reg {
	width: 32;
	instance_count: MAX_FLOWS;
}

action copy_seqno_action() {
	modify_field(tcp_re_m.seqno, tcp.seqNo);
}
ACTION2TABLE(copy_seqno)

blackbox stateful_alu check_seq_num_alu {
	reg: last_seq_reg;

	condition_lo: register_lo > tcp_re_m.seqno; //tcp.seqNo; // tcp_re_m.seqno;

	update_lo_1_value: tcp_re_m.seqno; //tcp.seqNo; //tcp_re_m.seqno;

	output_dst: tcp_re_m.retransmission;
	output_value: combined_predicate;
}
BLACKBOX2TABLE(check_seq_num)

blackbox stateful_alu add_retransmission_alu {
	reg: ret_count_reg;
	update_lo_1_value: register_lo + tcp_re_m.retransmission;
}
BLACKBOX2TABLE(add_retransmission)


control tcp_retransmission_features {
	if (valid(tcp)) {
		apply(check_seq_num_table);
		apply(add_retransmission_table);
	}
}