#ifndef CONTROLLER_PREPARE_DATAPLANE_H
#define CONTROLLER_PREPARE_DATAPLANE_H

#include "../config.h"

#include <tofinopd/marina_data_plane/pd/pd.h>

extern int16_t DEVPORT_MAPPING[MAX_DEV_PORT_ID];

void prepare_dataplane(p4_pd_sess_hdl_t sess_hdl);

#endif //CONTROLLER_PREPARE_DATAPLANE_H
