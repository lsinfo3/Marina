
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>


#include "../config.h"


uint32_t new[FEATURE_SIZE], old[FEATURE_SIZE], last[FEATURE_SIZE], values[FEATURE_SIZE];

void process() {
	for (int i = 0; i < FEATURE_SIZE; i++) {
		values[i] = new[i] - old[i];
	}
}

void age() {
	uint32_t now = 1234;

	for (int i = 0; i < FEATURE_SIZE; i++) {
		if (new[i] != old[i]) {
			last[i] = now;
		}
	}
}


int insert_into_queue(int i) {
	return i << 1;
}

void do_remove() {
	uint32_t now = 1235;
	for (int i = 0; i < FEATURE_SIZE; i++) {
		if (last[i] < now) {
			insert_into_queue(i);
		}
	}
}

void all() {
	uint32_t now = 1235;

	for (int i = 0; i < FEATURE_SIZE; i++) {
		values[i] = new[i] - old[i];
		if (new[i] != old[i]) {
			last[i] = now;
		} else if (last[i] < now) {
			insert_into_queue(i);
		}
	}
}

int main(void) {
	struct timeval start, end, diff;
	gettimeofday(&start, NULL);
	process();
	age();
	do_remove();
	gettimeofday(&end, NULL);
	timersub(&end, &start, &diff);
	printf("multi_loops %ld.%ld\n", diff.tv_sec, diff.tv_usec);

	gettimeofday(&start, NULL);
	all();
	gettimeofday(&end, NULL);
	timersub(&end, &start, &diff);
	printf("single_loop %ld.%ld\n", diff.tv_sec, diff.tv_usec);

	return 0;
}