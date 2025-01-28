#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include <BeDefs.h>
#include "debugbreak.h"
#include <haiku_errors.h>
#include "linux_syscall.h"

#define RANDOM_SYSCALLS "random"
#define RANDOM_GET_ENTROPY 1
struct random_get_entropy_args {
	void *buffer;
	size_t length;
};
static inline ssize_t _getrandom(void *buf, size_t len, int flags) {
	return LINUX_SYSCALL3(__NR_getrandom, buf, len, flags);
}
extern "C" {
int32_t _moni_generic_syscall(const char *userSubsystem, uint32_t function, void *buffer, uint64_t bufferSize) {
	char subsystem[B_FILE_NAME_LENGTH];
	if (!strcmp(userSubsystem, RANDOM_SYSCALLS)) {
		switch (function) {
			case RANDOM_GET_ENTROPY: {
				random_get_entropy_args args;
				if (bufferSize != sizeof(args)) {
					return B_BAD_VALUE;
				}
				memcpy(&args, buffer, sizeof(args));
				if (_getrandom(args.buffer, args.length, 0) < 0) {
					return B_IO_ERROR;
				}	
				memcpy(buffer, &args, sizeof(args));
				return B_OK;
			} break;
		}
	}
    	GET_HOSTCALLS()->printf("fatal stub: _kern_generic_syscall(subsystem: \"%s\". function: %0d, len: %0ld)\n", subsystem, function, bufferSize);
        GET_SERVERCALLS()->debug_output("fatal stub: _kern_generic_syscall", 33);
	return B_BAD_HANDLER;
}
}
