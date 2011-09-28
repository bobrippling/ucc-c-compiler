#include "stdlib.h"
#include "syscalls.h"

void exit(int code)
{
	__syscall(SYS_exit, code);
}
