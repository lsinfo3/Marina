#ifndef CONTROLLER_LOGGING_H
#define CONTROLLER_LOGGING_H

#define ERROR_LOG(str, ...) printf("ERROR: "__FILE__"@%d " str "\n", __LINE__, ## __VA_ARGS__);

#ifdef INFO_LOGGING
#define INFO_LOG(str, ...) printf("INFO: "__FILE__"@%d " str "\n", __LINE__, ## __VA_ARGS__);
#else
#define INFO_LOG(str, ...)
#endif

#ifdef DEBUG
#define DEBUG_LOG(str, ...) {                                                                   \
    time_t __debug_time = time(NULL);                                                           \
    printf("%.24s "__FILE__"@%d " str "\n", ctime(&__debug_time), __LINE__, ## __VA_ARGS__);    \
    }
#else
#define DEBUG_LOG(...)
#endif

#ifdef ENABLE_VERBOSE
#define VERBOSE_LOG(...) DEBUG_LOG(__VA_ARGS__)
#else
#define VERBOSE_LOG(...)
#endif

#define BENCH_PREPARE_FORCE(name) struct timeval _bench_##name##_start, _bench_##name##_end, _bench_##name##_diff;
#define BENCH_START_FORCE(name) gettimeofday(&_bench_##name##_start, NULL);
#define BENCH_END_FORCE(name, str) \
	gettimeofday(&_bench_##name##_end, NULL); \
	timersub(&_bench_##name##_end, &_bench_##name##_start, &_bench_##name##_diff); \
	printf(str ": %ld.%06lds\n", _bench_##name##_diff.tv_sec, _bench_##name##_diff.tv_usec);

#ifdef ENABLE_BENCH
#define BENCH_PREPARE(name) BENCH_PREPARE_FORCE(name)
#define BENCH_START(name) BENCH_START_FORCE(name)
#define BENCH_END(name, str) BENCH_END_FORCE(name, str)
#else
#define BENCH_PREPARE(name)
#define BENCH_START(name)
#define BENCH_END(name, str)
#endif


#endif //CONTROLLER_LOGGING_H
