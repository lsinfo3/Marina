#include <stdio.h>

#include <tofinopd/marina_data_plane/pd/pd.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/bf_pal/bf_pal_types.h>
#include <tofino/pdfixed/pd_tm.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>

#include "../config.h"

#define ARG_UNUSED __attribute__((unused))

#define print(str) VERBOSE_LOG(str)

pipe_status_t pipe_mgr_init(void) {
	print("INIT");
	return 0;
}

pipe_status_t pipe_mgr_client_init(pipe_sess_hdl_t *sess_hdl) {
	*sess_hdl = 0;
	print("CLIENT_INIT");
	return 0;
}

pipe_status_t pipe_stful_database_sync(pipe_sess_hdl_t sess_hdl ARG_UNUSED,
									   dev_target_t dev_tgt ARG_UNUSED,
									   pipe_stful_tbl_hdl_t stful_tbl_hdl ARG_UNUSED,
									   pipe_stful_tbl_sync_cback_fn cback_fn ARG_UNUSED,
									   void *cookie ARG_UNUSED) {
	return 0;
}

bf_status_t bf_pal_front_port_to_dev_port_get(
		bf_dev_id_t dev_id,
		bf_pal_front_port_handle_t *port_hdl,
		bf_dev_port_t *dev_port) { return 0; }

bf_status_t bf_pal_is_port_internal(bf_dev_id_t dev_id,
									bf_dev_port_t dev_port,
									bool *is_internal) { return 0; }

bf_status_t bf_pal_port_add(bf_dev_id_t dev_id,
							bf_dev_port_t dev_port,
							bf_port_speed_t speed,
							bf_fec_type_t fec_type) { return 0; }

bf_status_t bf_pal_port_enable(bf_dev_id_t dev_id, bf_dev_port_t dev_port) { return 0; }
bf_status_t bf_pal_port_del_all(bf_dev_id_t dev_id) {return 0;}

p4_pd_status_t p4_pd_tm_set_cpuport(p4_pd_tm_dev_t dev, p4_pd_tm_port_t port) { return 0; }

pipe_status_t pipe_mgr_client_cleanup(pipe_sess_hdl_t def_sess_hdl) { return 0; }

p4_pd_status_t p4_pd_marina_data_plane_iat_compute_log_table_table_add_with_iat_compute_log_action
		(
				p4_pd_sess_hdl_t sess_hdl,
				p4_pd_dev_target_t dev_tgt,
				p4_pd_marina_data_plane_iat_compute_log_table_match_spec_t *match_spec,
				p4_pd_marina_data_plane_iat_compute_log_action_action_spec_t *action_spec,
				p4_pd_entry_hdl_t *entry_hdl
		) { return 0; }

p4_pd_status_t p4_pd_marina_data_plane_compute_size_log_table_table_add_with_compute_size_log_action(
		p4_pd_sess_hdl_t sess_hdl,
		p4_pd_dev_target_t dev_tgt,
		p4_pd_marina_data_plane_compute_size_log_table_match_spec_t *match_spec,
		p4_pd_marina_data_plane_compute_size_log_action_action_spec_t *action_spec,
		p4_pd_entry_hdl_t *entry_hdl
) { return 0; }

p4_pd_status_t p4_pd_marina_data_plane_header_length_table_table_add_with_header_length_hit(
		p4_pd_sess_hdl_t sess_hdl,
		p4_pd_dev_target_t dev_tgt,
		p4_pd_marina_data_plane_header_length_table_match_spec_t *match_spec,
		int priority,
		p4_pd_marina_data_plane_header_length_hit_action_spec_t *action_spec,
		p4_pd_entry_hdl_t *entry_hdl
) { return 0; }

p4_pd_status_t p4_pd_marina_data_plane_forward_table_table_add_with_forward_action(
		p4_pd_sess_hdl_t sess_hdl,
		p4_pd_dev_target_t dev_tgt,
		p4_pd_marina_data_plane_forward_table_match_spec_t *match_spec,
		p4_pd_marina_data_plane_forward_action_action_spec_t *action_spec,
		p4_pd_entry_hdl_t *entry_hdl
) { return 0; }

#define MOCK_WRITE(name, type) \
p4_pd_status_t p4_pd_marina_data_plane_register_write_##name (       \
        p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,                 \
        p4_pd_dev_target_t dev_tgt ARG_UNUSED,                \
        int index ARG_UNUSED,                                 \
        type *register_value ARG_UNUSED) {                 \
    print("write" #name);                                     \
    return 0;                                                 \
}

#define MOCK_READ(name, type)                                      \
p4_pd_status_t p4_pd_marina_data_plane_register_range_read_##name (  \
        p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,                 \
        p4_pd_dev_target_t dev_tgt ARG_UNUSED,                \
        int index ARG_UNUSED,                                 \
        int count ARG_UNUSED,                                 \
        int flags ARG_UNUSED,                                 \
        int *num_actually_read ARG_UNUSED,                    \
        type *register_values ARG_UNUSED,                  \
        int *value_count ARG_UNUSED) {                        \
    print("RANGE_READ " #name);                               \
    return 0;                                                 \
}

#define MOCK_RESET_ALL(name)                                  \
p4_pd_status_t p4_pd_marina_data_plane_register_reset_all_##name (   \
        p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,                 \
        p4_pd_dev_target_t dev_tgt ARG_UNUSED) {              \
    print("RESET" #name);                                     \
    return 0;                                                 \
}

#define MOCK_REG(name, type) MOCK_READ(name, type) MOCK_WRITE(name, type) MOCK_RESET_ALL(name)

MOCK_REG(pkt_count_reg, uint32_t)

MOCK_REG(size_log_reg, uint32_t)

MOCK_REG(size_log_square_reg, uint32_t)
MOCK_REG(size_log_cube_reg, uint32_t)

MOCK_REG(iat_log_reg, uint32_t)

MOCK_REG(iat_last_timestamp_reg, uint32_t)

//MOCK_REG(iat_log_min_reg, uint8_t)

//MOCK_REG(iat_log_max_reg, uint8_t)

MOCK_REG(iat_log_square_reg, uint32_t)
MOCK_REG(iat_log_cube_reg, uint32_t)

MOCK_REG(ret_count_reg, uint32_t)

#define MOCK_TABLE_GET_ENTRY_COUNT(name)\
p4_pd_status_t p4_pd_marina_data_plane_##name##_get_entry_count(\
                p4_pd_sess_hdl_t sess_hdl,\
                p4_pd_dev_target_t dev_tgt,\
                uint32_t *count \
        ) {*count = 0;return 0;}

#define MOCK_TABLE_GET_FIRST_ENTRY_HANDLE(name)\
p4_pd_status_t p4_pd_marina_data_plane_##name##_get_first_entry_handle(\
                p4_pd_sess_hdl_t sess_hdl,\
                p4_pd_dev_target_t dev_tgt,\
                int *index\
        ) {*index = 0;return 0;}

#define MOCK_TABLE_GET_NEXT_ENTRY_HANDLES(name) \
p4_pd_status_t p4_pd_marina_data_plane_##name##_get_next_entry_handles( \
                p4_pd_sess_hdl_t sess_hdl,\
                p4_pd_dev_target_t dev_tgt,\
                p4_pd_entry_hdl_t entry_handle,\
                int n,\
                int *next_entry_handles\
        ) {*next_entry_handles=0;return 0;}

#define MOCK_TABLE_DELETE(name) \
p4_pd_status_t p4_pd_marina_data_plane_##name##_table_delete( \
                p4_pd_sess_hdl_t sess_hdl, \
                uint8_t dev_id, \
                p4_pd_entry_hdl_t entry_hdl \
        ) {return 0;}

#define MOCK_TABLE_RESET_DEFAULT(name)\
p4_pd_status_t p4_pd_marina_data_plane_##name##_table_reset_default_entry(\
                p4_pd_sess_hdl_t sess_hdl,\
                p4_pd_dev_target_t pd_dev_tgt\
        ) {return 0;}

#define MOCK_TABLE(name) \
    MOCK_TABLE_GET_ENTRY_COUNT(name) \
    MOCK_TABLE_GET_FIRST_ENTRY_HANDLE(name) \
    MOCK_TABLE_GET_NEXT_ENTRY_HANDLES(name) \
    MOCK_TABLE_DELETE(name) \
    MOCK_TABLE_RESET_DEFAULT(name)

MOCK_TABLE(forward_table)

MOCK_TABLE(iat_compute_log_table)

MOCK_TABLE(compute_size_table)

MOCK_TABLE(header_length_table)

MOCK_TABLE(compute_size_log_table)

#define MOCK_SET_PROPERTY(name) \
p4_pd_status_t p4_pd_marina_data_plane_##name##_set_property ( \
                p4_pd_sess_hdl_t sess_hdl, \
                uint8_t dev_id, \
                p4_pd_tbl_prop_type_t property, \
                p4_pd_tbl_prop_value_t value, \
                p4_pd_tbl_prop_args_t args \
        ) {return 0;}

MOCK_SET_PROPERTY(add_size_log_table)
MOCK_SET_PROPERTY(add_size_log_square_table)
MOCK_SET_PROPERTY(add_size_log_cube_table)

MOCK_SET_PROPERTY(add_iat_log_table)
MOCK_SET_PROPERTY(add_iat_log_square_table)
MOCK_SET_PROPERTY(add_iat_log_cube_table)

MOCK_SET_PROPERTY(pkt_count_table)

MOCK_SET_PROPERTY(add_retransmission_table)
MOCK_SET_PROPERTY(check_seq_num_table)

MOCK_SET_PROPERTY(iat_read_update_last_timestamp_table)

//MOCK_SET_PROPERTY(iat_log_min_table)

//MOCK_SET_PROPERTY(iat_log_max_table)

MOCK_SET_PROPERTY(classification_ipv4)

MOCK_SET_PROPERTY(bloom_filter_1_table)
MOCK_SET_PROPERTY(bloom_filter_2_table)
MOCK_SET_PROPERTY(bloom_filter_3_table)
MOCK_SET_PROPERTY(bloom_filter_4_table)

#ifdef ENABLE_IP6
MOCK_SET_PROPERTY(classification_ipv6)
#endif


p4_pd_status_t p4_pd_marina_data_plane_classification_ipv4_table_add_with_classification_hit_action(
		p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,
		p4_pd_dev_target_t dev_tgt ARG_UNUSED,
		p4_pd_marina_data_plane_classification_ipv4_match_spec_t *match_spec ARG_UNUSED,
		p4_pd_marina_data_plane_classification_hit_action_action_spec_t *action_spec ARG_UNUSED,
		p4_pd_entry_hdl_t *entry_hdl) {
	print("INSERT_IPV4");
	*entry_hdl = 0;
	return 0;
}

#ifdef ENABLE_IP6
p4_pd_status_t p4_pd_marina_data_plane_classification_ipv6_table_add_with_classification_hit_action(
		p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,
		p4_pd_dev_target_t dev_tgt ARG_UNUSED,
		p4_pd_marina_data_plane_classification_ipv6_match_spec_t *match_spec ARG_UNUSED,
		p4_pd_marina_data_plane_classification_hit_action_action_spec_t *action_spec ARG_UNUSED,
		p4_pd_entry_hdl_t *entry_hdl) {
	print("INSERT_IPV6");
	*entry_hdl = 0;
	return 0;
}
#endif

p4_pd_status_t p4_pd_marina_data_plane_classification_ipv4_table_delete(
		p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,
		uint8_t dev_id ARG_UNUSED,
		p4_pd_entry_hdl_t entry_hdl ARG_UNUSED) {
	print("DELETE_IPV4");
	return 0;
}

p4_pd_status_t p4_pd_marina_data_plane_classification_ipv6_table_delete(
		p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,
		uint8_t dev_id ARG_UNUSED,
		p4_pd_entry_hdl_t entry_hdl ARG_UNUSED) {
	print("DELETE_IPV6");
	return 0;
}

#define MOCK_WRITE_BLOOM(id)                                        \
p4_pd_status_t p4_pd_marina_data_plane_register_write_bloom_filter_##id (  \
        p4_pd_sess_hdl_t sess_hdl ARG_UNUSED,                       \
        p4_pd_dev_target_t dev_tgt ARG_UNUSED,                      \
        int index ARG_UNUSED,                                       \
        uint8_t *register_value ARG_UNUSED) {                       \
    print("WRITE_BLOOM " #id);                                      \
    return 0;                                                       \
}

MOCK_WRITE_BLOOM(1)

MOCK_WRITE_BLOOM(2)

MOCK_WRITE_BLOOM(3)

MOCK_WRITE_BLOOM(4)
