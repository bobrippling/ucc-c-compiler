#include "../syscalls.h"
#include "time.h"

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	return __syscall(SYS_nanosleep, req, rem);
}
