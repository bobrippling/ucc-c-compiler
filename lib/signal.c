#include "signal.h"
#include "syscalls.h"

int kill(pid_t pid, int sig)
{
	return __syscall(SYS_kill, pid, sig);
}

int raise(int sig)
{
	return kill(getpid(), sig);
}
