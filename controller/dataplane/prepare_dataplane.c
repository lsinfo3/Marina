#include "prepare_dataplane.h"

#include <stdlib.h>

#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/pdfixed/pd_tm.h>

#include "../port_config.h"
#include "dp_api.h"

#include "../static_tables.h"

#define DP_EXIT_ON_ERROR(call, errstring) { \
	p4_pd_status_t status;                  \
	status = call;                          \
	if (status != 0) {                      \
    	ERROR_LOG(errstring " %s\n", pipe_str_err(status))     \
    	exit(1);\
	}\
}

#define CLEAR_TABLE(TABLE) { \
    DP_EXIT_ON_ERROR(p4_pd_marina_data_plane_##TABLE##_get_entry_count(sess_hdl, ALL_PIPES, &count), \
    	"Error when getting " #TABLE " entry count")	                         \
    int forward_entry_hdls[MAX_FLOWS];\
    p4_pd_marina_data_plane_##TABLE##_get_first_entry_handle(sess_hdl, ALL_PIPES, &forward_entry_hdls[0]);\
    if (count > 1) {\
        p4_pd_marina_data_plane_##TABLE##_get_next_entry_handles(sess_hdl, ALL_PIPES, forward_entry_hdls[0],\
            (int) count - 1, forward_entry_hdls + 1);\
    }\
    for (int i = 0; i < count; i++) {\
        p4_pd_marina_data_plane_##TABLE##_table_delete(sess_hdl, 0, forward_entry_hdls[i]);\
    }\
    p4_pd_marina_data_plane_##TABLE##_table_reset_default_entry(sess_hdl, ALL_PIPES);       \
}

#define SET_ASSYM(TABLE) { \
    p4_pd_tbl_prop_args_t args = { .value = 0 }; \
    p4_pd_tbl_prop_value_t value = { .scope = PD_ENTRY_SCOPE_SINGLE_PIPELINE }; \
    p4_pd_marina_data_plane_##TABLE##_set_property(sess_hdl, 0, PD_TABLE_ENTRY_SCOPE, value, args); \
}

int16_t DEVPORT_MAPPING[MAX_DEV_PORT_ID];

static bf_dev_port_t front_panel_to_devport(uint8_t port, uint8_t lane) {
	bf_pal_front_port_handle_t front_port_handle = {
			.conn_id = port,
			.chnl_id = lane,
	};
	bf_dev_port_t dev_port;
	DP_EXIT_ON_ERROR(bf_pal_front_port_to_dev_port_get(0, &front_port_handle, &dev_port),
					 "when converting front panel to dev port");
	return dev_port;
}

void prepare_dataplane(p4_pd_sess_hdl_t sess_hdl) {
	uint32_t count;
	p4_pd_entry_hdl_t entry_hdl;

	bf_pal_port_del_all(0);

	CLEAR_TABLE(forward_table)
	CLEAR_TABLE(iat_compute_log_table)
	CLEAR_TABLE(compute_size_log_table)
	CLEAR_TABLE(header_length_table)

	SET_ASSYM(pkt_count_table)

	SET_ASSYM(bloom_filter_1_table)
	SET_ASSYM(bloom_filter_2_table)
	SET_ASSYM(bloom_filter_3_table)
	SET_ASSYM(bloom_filter_4_table)

	SET_ASSYM(add_size_log_table)
	SET_ASSYM(add_size_log_square_table)
	SET_ASSYM(add_size_log_cube_table)

	SET_ASSYM(iat_read_update_last_timestamp_table)
	SET_ASSYM(add_iat_log_table)
	SET_ASSYM(add_iat_log_square_table)
	SET_ASSYM(add_iat_log_cube_table)

//	SET_ASSYM(add_retransmission_table)
//	SET_ASSYM(check_seq_num_table)
//	SET_ASSYM(ack_count_table)
//	SET_ASSYM(iat_log_min_table)
//	SET_ASSYM(iat_log_max_table)
#ifdef ENABLE_IP4
	SET_ASSYM(classification_ipv4)
#endif
#ifdef ENABLE_IP6
	SET_ASSYM(classification_ipv6)
#endif

	p4_pd_tm_set_cpuport(0, CPU_PORT);


	for (int i = 0; i < (sizeof(iat_log_square) / sizeof(log_square_entry_t)); i++) {
		p4_pd_marina_data_plane_iat_compute_log_table_match_spec_t match_spec = {
				.alu_iat_m_iat = iat_log_square[i].match,
				.alu_iat_m_iat_prefix_length = iat_log_square[i].mask,
		};
		p4_pd_marina_data_plane_iat_compute_log_action_action_spec_t action_spec = {
				.action_log = iat_log_square[i].value,
				.action_log_square = iat_log_square[i].square,
				.action_log_cube = iat_log_square[i].cube,
		};

		DP_EXIT_ON_ERROR(p4_pd_marina_data_plane_iat_compute_log_table_table_add_with_iat_compute_log_action(
				sess_hdl, ALL_PIPES, &match_spec, &action_spec, &entry_hdl
		), "when inserting iat_log entry");
	}

	for (int i = 0; i < (sizeof(size_log_square) / sizeof(log_square_entry_t)); i++) {
		p4_pd_marina_data_plane_compute_size_log_table_match_spec_t match_spec = {
				.alu_size_m_packet_size = size_log_square[i].match,
				.alu_size_m_packet_size_prefix_length = size_log_square[i].mask
		};
		p4_pd_marina_data_plane_compute_size_log_action_action_spec_t action_spec = {
				.action_log = size_log_square[i].value,
				.action_log_square = size_log_square[i].square,
				.action_log_cube = size_log_square[i].cube,
		};

		DP_EXIT_ON_ERROR(p4_pd_marina_data_plane_compute_size_log_table_table_add_with_compute_size_log_action(
				sess_hdl, ALL_PIPES, &match_spec, &action_spec, &entry_hdl
		), "when inserting size_log entry");
	}

	for (int i = 0; i < sizeof(header_length_match) / sizeof(p4_pd_marina_data_plane_header_length_table_match_spec_t); i++) {
		DP_EXIT_ON_ERROR(p4_pd_marina_data_plane_header_length_table_table_add_with_header_length_hit(
				sess_hdl, ALL_PIPES, &header_length_match[i], i, &header_length_action[i], &entry_hdl
		), "when inserting header length entry");
	}

	for (int i = 0; i < sizeof(port_entries) / sizeof(port_entry_t); i++) {
		bf_dev_port_t dev_port = front_panel_to_devport(port_entries[i].port, port_entries[i].lane);

		bool is_internal;
		DP_EXIT_ON_ERROR(bf_pal_is_port_internal(0, dev_port, &is_internal),
						 "when checking whether port is internal");
		if (is_internal) {
			ERROR_LOG("Can not configure port %d:%d as it is internal",
					  port_entries[i].port, port_entries[i].lane)
			exit(1);
		}

		DP_EXIT_ON_ERROR(bf_pal_port_add(0, dev_port, port_entries[i].speed, port_entries[i].fec), "when adding port")
		DP_EXIT_ON_ERROR(bf_pal_port_enable(0, dev_port), "when enabling port")
	}

	for (int i=0; i<MAX_DEV_PORT_ID; i++) {
		DEVPORT_MAPPING[i] = -1;
	}

	for (int i = 0; i < sizeof(forward_entries) / sizeof(forward_entry_t); i++) {
		bf_dev_port_t from_port = front_panel_to_devport(forward_entries[i].from_port, forward_entries[i].from_lane);
		bf_dev_port_t to_port = front_panel_to_devport(forward_entries[i].to_port, forward_entries[i].to_lane);

		DEVPORT_MAPPING[from_port] = (int16_t) to_port;

		p4_pd_marina_data_plane_forward_table_match_spec_t match_spec = {
				.ig_intr_md_ingress_port = from_port
		};
		p4_pd_marina_data_plane_forward_action_action_spec_t action_spec = {
				.action_port = to_port
		};

		DP_EXIT_ON_ERROR(
				p4_pd_marina_data_plane_forward_table_table_add_with_forward_action(sess_hdl, ALL_PIPES, &match_spec,
																		  &action_spec, &entry_hdl),
				"when adding forward entry"
		);
	}
}