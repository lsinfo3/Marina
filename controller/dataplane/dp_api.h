#ifndef LOW_LEVEL_LIB_DP_API_H
#define LOW_LEVEL_LIB_DP_API_H

#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofinopd/marina_data_plane/pd/pd.h>

#include "../datastructures/structs.h"
#include "../config.h"


#define SUCCESS 0
#define ERR_CONNECT 1
#define ERR_PROTO 2
#define ERR_SWITCH 3

extern char *ERR_TO_STR[];

extern p4_pd_dev_target_t ALL_PIPES;
extern p4_pd_dev_target_t PIPES[];


int switch_connect(pipe_sess_hdl_t *sess_hdl);

int switch_disconnect(pipe_sess_hdl_t sess_hdl);

int switch_add_classification_entry(const five_tuple_t *five_tuple, pipe_sess_hdl_t sess_hdl,
									uint32_t register_index, p4_pd_entry_hdl_t *entry_hdl);

int
switch_remove_classification_entry(pipe_sess_hdl_t sess_hdl, const five_tuple_t *five_tuple,
								   p4_pd_entry_hdl_t entry_hdl);

int
switch_write_bloom_filter(pipe_sess_hdl_t sess_hdl, int pipe_id, const int bloom_filter_indices[NUM_BLOOMFILTERS],
						  uint8_t value);

#endif //LOW_LEVEL_LIB_DP_API_H
