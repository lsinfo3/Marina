#ifndef LOW_LEVEL_LIB_FEATURES_H
#define LOW_LEVEL_LIB_FEATURES_H

#include <stdint.h>
#include <pthread.h>

#include <tofinopd/marina_data_plane/pd/pd.h>

#include "dp_api.h"

#include "../config.h"

typedef enum {
	PACKET_COUNT_REG,
	SIZE_LOG_REG,
	SIZE_LOG_SQUARE_REG,
	SIZE_LOG_CUBE_REG,
	IAT_LOG_REG,
	IAT_LOG_SQUARE_REG,
	IAT_LOG_CUBE_REG,
	//TCP_RETRANSMISSION,
	NUM_FEATURES,
} register_ids_t;


typedef p4_pd_status_t (*register_range_read_fn_uint32_t)(
		p4_pd_sess_hdl_t sess_hdl,
		p4_pd_dev_target_t dev_tgt,
		int index,
		int count,
		int flags,
		int *num_actually_read,
		uint32_t *register_values,
		int *value_count
);


typedef p4_pd_status_t (*register_write_fn_uint32_t)(
		p4_pd_sess_hdl_t sess_hdl,
		p4_pd_dev_target_t dev_tgt,
		int index,
		uint32_t *register_value
);

/*
 * Holds the access and write functions for a register
 * as well as a shadow copy of the data plane registers
 * and the features for each flow
 */
typedef struct feature_register_t {
	register_ids_t reg_id;
	char *name;
	uint8_t on_egress;
	int table_hdl;

	register_range_read_fn_uint32_t read_fn;
	register_write_fn_uint32_t write_fn;

	p4_pd_status_t (*reset_fn)(
			p4_pd_sess_hdl_t sess_hdl,
			p4_pd_dev_target_t dev_tgt
	);

	void (*process_register) (
			struct feature_register_t *register_info
	);

	uint32_t *data_plane_values;
	uint32_t *last_data_plane_value;
	uint32_t buffer_a[FEATURE_SIZE];
	uint32_t buffer_b[FEATURE_SIZE];
	uint64_t values[FEATURE_SIZE];
} feature_register_t;

extern feature_register_t register_count;
extern feature_register_t register_size_log;
extern feature_register_t register_size_log_square;
extern feature_register_t register_size_log_cube;
extern feature_register_t register_iat_log;
extern feature_register_t register_iat_log_square;
extern feature_register_t register_iat_log_cube;
extern feature_register_t register_last_timestamp;
//extern feature_register_t register_tcp_retransmission;

#define STATIC_FEATURES_ARRAY static feature_register_t *features[NUM_FEATURES] = { \
    &register_count,                                                                \
    &register_size_log,                                                             \
    &register_size_log_square,                                                      \
    &register_size_log_cube,                                                        \
    &register_iat_log,                                                              \
    &register_iat_log_square,                                                       \
    &register_iat_log_cube,                                                         \
};

void prepare_features(pipe_sess_hdl_t sess_hdl);

void read_features(pipe_sess_hdl_t sess_hdl);

void set_new_flow_initial_timeout(uint32_t flow_id);

extern volatile bool dataplane_new_features;
extern pthread_spinlock_t dataplane_feature_lock;
extern pthread_cond_t dataplane_new_features_cond;
extern pthread_mutex_t dataplane_new_features_lock;

#endif //LOW_LEVEL_LIB_FEATURES_H
