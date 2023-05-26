#ifndef LOW_LEVEL_LIB_BLOOM_FILTER_H
#define LOW_LEVEL_LIB_BLOOM_FILTER_H

#include "structs.h"

void bloom_filter_init();

void bloom_filter_set_irrelevant(const five_tuple_t *five_tuple);

void bloom_filter_set_relevant(const five_tuple_t *five_tuple);

_Noreturn void *bloom_filter_maintenance(__attribute__((unused)) void *arg);

#endif //LOW_LEVEL_LIB_BLOOM_FILTER_H
