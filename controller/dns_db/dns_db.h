#ifndef CONTROLLER_DNS_DB_H
#define CONTROLLER_DNS_DB_H

#include <stdint.h>
#include "../logging/statistics.h"

GLOBAL_STAT_HDR(dnsdb)


void *dns_db_thread(void *args);

#endif //CONTROLLER_DNS_DB_H
