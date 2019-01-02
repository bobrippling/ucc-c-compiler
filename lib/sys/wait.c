#include "wait.h"
#include "../syscalls.h"
#include "../unistd.h"

int fork(void)
{
	return __syscall(SYS_fork);
}

pid_t waitpid(int pid, int *status, int options)
{
	return __syscall(SYS_wait4, pid, status, options, NULL);
}

int WEXITSTATUS(int stat)
{
	return (stat & 0xff00) >> 8;
}

/*pid_t wait(int *status)
{
	return waitpid(-1, status, 0);
}*/
