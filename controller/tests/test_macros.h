#ifndef LOW_LEVEL_LIB_TEST_MACROS_H
#define LOW_LEVEL_LIB_TEST_MACROS_H

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>

#define PREPARE_TIMING struct timeval now, last, diff; gettimeofday(&last, NULL);
#define PRINT_TIME_DELTA(msg) gettimeofday(&now, NULL); timersub(&now, &last, &diff); printf(msg " Timedelta %ld.%06lds\n", diff.tv_sec, diff.tv_usec); last = now;

#define ASSERT_EQUAL(value, expected) if ((expected) != (value)) {printf("expected %x but got %x on line %d in file %s\n", (expected), (value), __LINE__, __FILE__);raise(SIGINT);}

#endif //LOW_LEVEL_LIB_TEST_MACROS_H
