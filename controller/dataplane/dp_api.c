#include "dp_api.h"

#include <stdio.h>

char *ERR_TO_STR[] = {
		"Success",
		"Connection error",
		"Wrong protocol provided",
		"Error from switch"
};

p4_pd_dev_target_t PIPES[] = {
		{.device_id = 0, .dev_pipe_id = 0},
		{.device_id = 0, .dev_pipe_id = 1},
		{.device_id = 0, .dev_pipe_id = 2},
		{.device_id = 0, .dev_pipe_id = 3},
};

p4_pd_dev_target_t ALL_PIPES = {
		.device_id = 0,
		.dev_pipe_id = PD_DEV_PIPE_ALL
};

/*
 * Initialize the switch client and retrieve a session id
 */
int switch_connect(pipe_sess_hdl_t *sess_hdl) {
	*sess_hdl = 0;
	pipe_status_t status;
	if ((status = pipe_mgr_init())) {
		printf("Could not init pipe_mgr, status: %s\n", pipe_str_err(status));
		return ERR_CONNECT;
	}
	if ((status = pipe_mgr_client_init(sess_hdl))) {
		printf("Could not get sess_hdl, status: %s\n", pipe_str_err(status));
		return ERR_CONNECT;
	}
	return SUCCESS;
}

int switch_disconnect(pipe_sess_hdl_t sess_hdl) {
	pipe_status_t status;
	if ((status = pipe_mgr_client_cleanup(sess_hdl))) {
		printf("Could not cleanup sess_hdl, status: %s\n", pipe_str_err(status));
		return ERR_CONNECT;
	}
	return SUCCESS;
}

/*
 * Add a classification entry for ipv4
 */
#ifdef ENABLE_IP4
static inline int
switch_add_ipv4_entry(const five_tuple_t *five_tuple, const pipe_sess_hdl_t sess_hdl,
					  const p4_pd_dev_target_t devTarget, const uint32_t register_index,
					  p4_pd_entry_hdl_t *entryHdl) {
	if (five_tuple->h.l3_proto != L3_IPV4)
		return ERR_PROTO;

	p4_pd_marina_data_plane_classification_ipv4_match_spec_t match_spec = {
			.ipv4_srcAddr = SWAP_ENDIAN_32(five_tuple->h.src_addr.ipv4),
			.ipv4_dstAddr = SWAP_ENDIAN_32(five_tuple->h.dst_addr.ipv4),
			.l4_lookup_src = five_tuple->h.src_port,
			.l4_lookup_dst = five_tuple->h.dst_port,
			.tcp_valid = five_tuple->h.l4_proto,
	};
	p4_pd_marina_data_plane_classification_hit_action_action_spec_t action_spec = {
			register_index,
	};
	pipe_status_t status = p4_pd_marina_data_plane_classification_ipv4_table_add_with_classification_hit_action(
			sess_hdl,
			devTarget,
			&match_spec,
			&action_spec,
			entryHdl
	);
	if (status) {
		printf("Could not insert ipv4_classif, status: %s\n", pipe_str_err(status));
		return ERR_SWITCH;
	}
	return SUCCESS;
}
#endif

#ifdef ENABLE_IP6
/*
 * Add a classification entry for ipv6
 */
static inline int
switch_add_ipv6_entry(const five_tuple_t *five_tuple, const pipe_sess_hdl_t sess_hdl,
					  const p4_pd_dev_target_t devTarget, const uint32_t register_index,
					  p4_pd_entry_hdl_t *entryHdl) {
	if (five_tuple->h.l3_proto != L3_IPV6) {
		return ERR_PROTO;
	}

	const uint8_t *src = five_tuple->h.src_addr.ipv6.s6_addr;
	const uint8_t *dst = five_tuple->h.dst_addr.ipv6.s6_addr;
	p4_pd_marina_data_plane_classification_ipv6_match_spec_t match_spec = {
			.ipv6_srcAddr = {
					src[15], src[14], src[13], src[12], src[11], src[10], src[9], src[8],
					src[7], src[6], src[5], src[4], src[3], src[2], src[1], src[0]
			},
			.ipv6_dstAddr = {
					dst[15], dst[14], dst[13], dst[12], dst[11], dst[10], dst[9], dst[8],
					dst[7], dst[6], dst[5], dst[4], dst[3], dst[2], dst[1], dst[0]
			},
			.l4_lookup_src = five_tuple->h.src_port,
			.l4_lookup_dst = five_tuple->h.dst_port,
			.tcp_valid = five_tuple->h.l4_proto,
	};

	p4_pd_marina_data_plane_classification_hit_action_action_spec_t action_spec = {
			register_index,
	};
	pipe_status_t status = p4_pd_marina_data_plane_classification_ipv6_table_add_with_classification_hit_action(
			sess_hdl,
			devTarget,
			&match_spec,
			&action_spec,
			entryHdl
	);
	if (status) {
		printf("Could not insert ipv6_classif, status: %s\n", pipe_str_err(status));
		return ERR_SWITCH;
	}
	return SUCCESS;
}
#endif

/*
 * Add a classification entry for the given five tuple into the correct pipe
 */
int switch_add_classification_entry(const five_tuple_t *five_tuple, const pipe_sess_hdl_t sess_hdl,
									const uint32_t register_index, p4_pd_entry_hdl_t *entry_hdl) {
	switch (five_tuple->h.l3_proto) {
#ifdef ENABLE_IP4
		case L3_IPV4:
			return switch_add_ipv4_entry(five_tuple, sess_hdl,
										 PIPES[dev_port_to_pipe(five_tuple->dev_port)],
										 register_index, entry_hdl);
#endif
#ifdef ENABLE_IP6
		case L3_IPV6:
			return switch_add_ipv6_entry(five_tuple, sess_hdl,
										 PIPES[dev_port_to_pipe(five_tuple->dev_port)],
										 register_index, entry_hdl);
#endif
		default:
			return ERR_PROTO;
	}
}

#ifdef ENABLE_IP4
static int
switch_remove_ipv4_entry(const pipe_sess_hdl_t sess_hdl, const p4_pd_entry_hdl_t entryHdl) {

	pipe_status_t status = p4_pd_marina_data_plane_classification_ipv4_table_delete(sess_hdl,
																			 ALL_PIPES.device_id,
																			 entryHdl);
	if (status) {
		printf("Could not remove ipv4_classif, status: %s\n", pipe_str_err(status));
		return ERR_SWITCH;
	}
	return SUCCESS;
}
#endif

#ifdef ENABLE_IP6
static int switch_remove_ipv6_entry(const pipe_sess_hdl_t sess_hdl, const p4_pd_entry_hdl_t entryHdl) {
	pipe_status_t status = p4_pd_marina_data_plane_classification_ipv6_table_delete(sess_hdl, ALL_PIPES.device_id,
																			 entryHdl);

	if (status) {
		printf("Could not remove ipv6_classif, status: %s\n", pipe_str_err(status));
		return ERR_SWITCH;
	}
	return SUCCESS;
}
#endif

/*
 * Remove the
 */
int
switch_remove_classification_entry(const pipe_sess_hdl_t sess_hdl, const five_tuple_t *five_tuple,
								   const p4_pd_entry_hdl_t entry_hdl) {
	switch (five_tuple->h.l3_proto) {
#ifdef ENABLE_IP4
		case L3_IPV4:
			return switch_remove_ipv4_entry(sess_hdl, entry_hdl);
#endif
#ifdef ENABLE_IP6
		case L3_IPV6:
			return switch_remove_ipv6_entry(sess_hdl, entry_hdl);
#endif
		default:
			return ERR_PROTO;
	}
}

typedef p4_pd_status_t (bloom_filter_write_fn_t)(
		p4_pd_sess_hdl_t sess_hdl,
		p4_pd_dev_target_t dev_tgt,
		int index,
		uint8_t *register_value
);

int switch_write_bloom_filter(const pipe_sess_hdl_t sess_hdl, const int pipe_id,
							  const int bloom_filter_indices[NUM_BLOOMFILTERS], uint8_t value) {
	static bloom_filter_write_fn_t *write_functions[] = {
			p4_pd_marina_data_plane_register_write_bloom_filter_1,
			p4_pd_marina_data_plane_register_write_bloom_filter_2,
			p4_pd_marina_data_plane_register_write_bloom_filter_3,
			p4_pd_marina_data_plane_register_write_bloom_filter_4
	};
	p4_pd_dev_target_t dev_target = PIPES[pipe_id];

	for (int i=0; i<NUM_BLOOMFILTERS; i++) {
		int index = bloom_filter_indices[i];
		if (index >= 0) {
			pipe_status_t status = write_functions[i](
					sess_hdl,
					dev_target,
					index,
					&value
			);
			if (status) {
				printf("Could not write bloom filter entry %d[%d] = %d, status: %s\n", i, index, value,
					   pipe_str_err(status));
				return ERR_SWITCH;
			}
		}
	}
	return SUCCESS;
}
