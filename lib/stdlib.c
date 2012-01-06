#include "stdlib.h"
#include "unistd.h"
#include "syscalls.h"
#include "signal.h"

void exit(int code)
{
	__syscall(SYS_exit, code);
}

int atoi(char *s)
{
	int i = 0;

	while(*s)
		if('0' <= *s && *s <= '9')
			i = 10 * i + *s++ - '0';
		else
			break;

	return i;
}

void *malloc(size_t size)
{
	return sbrk(size);
}

void free(void *p)
{
	/* no op... :C */
}

void abort()
{
	// TODO: unblock SIGABRT
	raise(SIGABRT);

	// TODO: restore + unblock SIGABRT and re-raise
}
