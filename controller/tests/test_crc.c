#include "../datastructures/hashes/crc.h"

#include <stdio.h>

#define ASSERT_EQUAL(value, expected) if ((expected) != (value)) {printf("expected %u but got %u on line %d in file %s\n", (expected), (value), __LINE__, __FILE__);}

int main(void) {
	char text[] = "abcd";

	printf("Start crc tests\n");
	ASSERT_EQUAL(crc_fn[0]((uint8_t *) text, 4, 0), 3984772369u)
	ASSERT_EQUAL(crc_fn[1]((uint8_t *) text, 4, 0), 2462583345u)
	ASSERT_EQUAL(crc_fn[2]((uint8_t *) text, 4, 0), 2599230614u)
	ASSERT_EQUAL(crc_fn[3]((uint8_t *) text, 4, 0), 2575044939u)
	printf("Success\n");

	typedef struct __attribute__((__packed__)) {
		uint32_t src_addr;
		uint32_t dst_addr;
		uint16_t src_port;
		uint16_t dst_port;
	} dp_five_tuple_t;

	dp_five_tuple_t ft = {
			.src_addr = 100,
			.dst_addr = 200,
			.src_port = 128,
			.dst_port = 128,
	};

	printf("%x\n", (int)(crc_fn[0]((uint8_t *) &ft, sizeof(dp_five_tuple_t), 0) & 0xfffff));
}