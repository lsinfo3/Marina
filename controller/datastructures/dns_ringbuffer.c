#include "dns_ringbuffer.h"

dns_ringbuffer_t dns_ringbuffer;

GLOBAL_STAT_IMPL(dns_ringbuffer)

MK_RINGBUFFER_IMPL(dns, dns_item_t, DNS_RINGBUFFER_SIZE, dns_ringbuffer)