#ifndef LOW_LEVEL_LIB_EXPORT_FEATURES_H
#define LOW_LEVEL_LIB_EXPORT_FEATURES_H

#include <stdint.h>
#include <bits/types/FILE.h>
#include <uuid/uuid.h>
#include "../dataplane/features.h"
#include "../datastructures/session_hashtable.h"

_Noreturn void *feature_export_thread(void *arg);
FILE* open_output_file();
void print_header(FILE* file, feature_register_t* features[NUM_FEATURES]);
void print_flow_data(FILE* file, session_hashtable_t* hashtable, feature_register_t* features[NUM_FEATURES], char uuid_mapping_copy[FEATURE_SIZE][UUID_STR_LEN]);

#endif //LOW_LEVEL_LIB_EXPORT_FEATURES_H
