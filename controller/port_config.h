#ifndef CONTROLLER_PORT_CONFIG_H
#define CONTROLLER_PORT_CONFIG_H


#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <bf_types/bf_types.h>

/*
 * Forwarding table
 */

typedef struct {
	uint8_t port;
	uint8_t lane;
	bf_port_speed_t speed;
	bf_fec_type_t fec;
} port_entry_t;

port_entry_t port_entries[] = {
		{1, 0, BF_SPEED_100G, BF_FEC_TYP_RS},
		{2, 0, BF_SPEED_100G, BF_FEC_TYP_RS},

};

typedef struct {
	uint8_t from_port;
	uint8_t from_lane;
	uint8_t to_port;
	uint8_t to_lane;
} forward_entry_t;

forward_entry_t forward_entries[] = {
		{1, 0, 2, 0},
		{2, 0, 1, 0},
};

#endif //CONTROLLER_PORT_CONFIG_H
