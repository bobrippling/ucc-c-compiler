//struct stat;
#include "../../lib/syscalls.h"

int stat(const char *path, struct stat *buf)
{
	return __syscall(SYS_stat, path, buf);
}
