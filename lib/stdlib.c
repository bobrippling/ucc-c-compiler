#include "stdlib.h"
#include "syscalls.h"
#include "unistd.h"

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

void *malloc(unsigned size)
{
	return sbrk(size);
}

void free(void *p)
{
	/* no op... :C */
}
