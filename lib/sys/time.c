#include "../syscalls.h"
#include "time.h"
#include "../errno.h"

int nanosleep(const struct timespec *req, struct timespec *rem)
{
#ifdef SYS_nanosleep
	return __syscall(SYS_nanosleep, req, rem);
#else
	(void)req;
	(void)rem;
	errno = ENOSYS;
	return -1;
#endif
}
