#include "unistd.h"
#include "syscalls.h"

int read(int fd, void *p, int size)
{
	return __syscall(SYS_read, size, p, fd);
}

int write(int fd, void *p, int size)
{
	return __syscall(SYS_write, size, p, fd);
}
