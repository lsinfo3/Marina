#ifndef LOW_LEVEL_LIB_CONFIG_H
#define LOW_LEVEL_LIB_CONFIG_H

#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include "datastructures/structs.h"

#include "../p4/config.p4"

#ifndef SHOW_STATISTICS
#define SHOW_STATISTICS
#endif

#ifndef INFO_LOGGING
#define INFO_LOGGING
#endif

/*
 * Verbose debug output, only active in debug mode
 * WARNING: very verbose
 */
#ifndef DEBUG
//#define DEBUG
#endif
#ifndef VERBOSE
//#define VERBOSE
#endif
#define BENCH
#define ENABLE_BENCH

/*
 * Do not listen for the data plane, instead insert fake packets
 */
#ifndef MOCK_PACKETS
#define MOCK_PACKETS
#endif

/*
 * Path to the fifo in which to write the features
 */
#define FEATURE_PIPE "/tmp/feature_pipe"

/*
 * Path to the fifo in which to write the relevant ip addresses
 */
#define DNS_PIPE "/tmp/dns_pipe"

/*
 * Interface which connects to the data plane
 */
#ifndef RUN_ON_MODEL
#define DP_IFACE "enp1s0"
#else
#define DP_IFACE "veth251"
#endif

/*
 * The number of available pipes on the data plane
 */
#define NUM_PIPES 4

/*
 * 32 bit seed for the murmur3 32bit hash function used in the hashtable
 */
#define MURMUR_SEED 0xDEADBEEF

/*
 * How many more slots than possible entries should the hashtable have?
 * 8 leads to moderate timings, taking up to .1 seconds to remove all entries from
 * the hashtable
 */
#define FLOW_HASHTABLE_MULTIPLIER 16

#define DNS_HASHTABLE_SIZE (1 << 16)

/*
 * Flow age timeout if features don't change
 */
#define FLOW_TIMEOUT 30

/*
 * Highest possible devport number (9 bits)
 */
#define MAX_DEV_PORT_ID 512

/*
 * Ringbuffer sizes
 */
#define DATAPLANE_RINGBUFFER_SIZE 1 << 12
#define PACKET_RINGBUFFER_SIZE 1 << 16


/*
 * Roughly once per second, each entry in each bloom filter will be decremented by one with the given probability
 * This mechanism implements a simple form of probabilistic flow aging in the bloom filter, without the need to keep
 * track of all inserted 5-tuples.
 * As each bloom filter entry tracks the number of 5-tuples inserted into it, a single subtraction will not necessarily
 * clear this bit.
 */
#define BLOOM_SUB_PROBABILITY 0.01

#define ENABLE_STATS

//#define EXPORT_NUMPY

/*
 * Enable periodic reporting of libpcap statistics
 */
//#define PCAP_STATISTICS

/*
 * Other global constants
 * DO NOT MODIFY
 */

#define FEATURE_SIZE (MAX_FLOWS * NUM_PIPES)
#define MIN_SESSION_ID 4

#ifndef NDEBUG
#define DEBUG
#endif

#ifdef DEBUG

#ifdef BENCH
#ifndef ENABLE_BENCH
#define ENABLE_BENCH
#endif
#endif

#ifdef VERBOSE
#define ENABLE_VERBOSE
#endif

#endif


#include "logging/logging.h"
#include "check_config.h"

#endif //LOW_LEVEL_LIB_CONFIG_H
