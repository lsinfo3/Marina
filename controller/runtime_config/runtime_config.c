#include <unistd.h>
#include "runtime_config.h"

static bool is_enabled(const char* option_name);
static bool file_exists(const char* file_name);

static bool use_sessions_cached = true;

void load_runtime_config() {
	use_sessions_cached = !is_enabled("controller_disable_sessions");
}

static bool is_enabled(const char *option_name) {
	// we only check if a file with the option_name exists in the working directory
	// change this in production code
	return file_exists(option_name);
}

static bool file_exists(const char *file_name) {
	// only works on POSIX platforms
	return access(file_name, F_OK) == 0;
}

bool use_sessions() {
	return use_sessions_cached;
}
