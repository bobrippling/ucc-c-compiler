#include "unistd.h"
#include "syscalls.h"

int read(int fd, void *p, int size)
{
	return __syscall(SYS_read, fd, p, size);
}

int write(int fd, void *p, int size)
{
	return __syscall(SYS_write, fd, p, size);
}
