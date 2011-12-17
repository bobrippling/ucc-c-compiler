#include "fcntl.h"
#include "../syscalls.h"

int open(const char *fname, int mode, ...)
{
	int flag = 0;
	if(mode & O_CREAT)
		flag = *(int *)(&mode + 1);
	return __syscall(SYS_open, fname, mode, flag);
}
