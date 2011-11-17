#include "fcntl.h"
#include "syscalls.h"

int open(const char *fname, int mode, int perm)
{
	return __syscall(SYS_open, fname, mode, perm);
}
