#ifndef LOW_LEVEL_LIB_CRC_H
#define LOW_LEVEL_LIB_CRC_H

#include <stdint.h>

uint32_t crc32_normal(const uint8_t *data, int len, uint32_t crc);

uint32_t crc32c(const uint8_t *data, int len, uint32_t crc);

uint32_t crc32d(const uint8_t *data, int len, uint32_t crc);

uint32_t crc32q(const uint8_t *data, int len, uint32_t crc);

typedef uint32_t (*crc_fn_t)(const uint8_t *data, int len, uint32_t crc);

extern crc_fn_t crc_fn[];


#endif //LOW_LEVEL_LIB_CRC_H
