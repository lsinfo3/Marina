#ifndef CONTROLLER_STATISTICS_H
#define CONTROLLER_STATISTICS_H

#include "../config.h"

_Noreturn void *statistics_thread(__attribute__((unused)) void *arg);

#define GLOBAL_STAT_HDR(name) extern uint32_t _stat_##name;
#define GLOBAL_STAT_IMPL(name) uint32_t _stat_##name;

#ifdef ENABLE_STATS
#define INC_STAT(name) _stat_##name++;
#else
#define INC_STAT(name)
#endif

#define PREP_READ_STAT(name) uint32_t _last_##name = 0;
#define READ_STAT(name, string) \
	INFO_LOG(string " %d (%d/s)", _stat_##name, _stat_##name - _last_##name); \
    _last_##name = _stat_##name;

#endif //CONTROLLER_STATISTICS_H
