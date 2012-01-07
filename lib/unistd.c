#include "unistd.h"
#include "syscalls.h"

static void *ucc_brk(void *p)
{
	return __syscall(SYS_brk, p);
}

int brk(void *p)
{
	void *ret;

	ret = ucc_brk(p);

	/* linux brk() returns the current break on failure */
	if(ret == p){
		return 0;
	}else{
		/* failure */
		/*errno = ENOMEM;*/
		return -1;
	}
}

void *sbrk(int inc)
{
	void *new;

	new = ucc_brk(NULL) + inc;

	if(brk(new) == -1){
		/*errno = ENOMEM;*/
		return (void *)-1;
	}

	return new;
}

pid_t getpid()
{
	return __syscall(SYS_getpid);
}

int read(int fd, void *p, int size)
{
	return __syscall(SYS_read, fd, p, size);
}

int write(int fd, void *p, int size)
{
	return __syscall(SYS_write, fd, p, size);
}

int close(int fd)
{
	return __syscall(SYS_close, fd);
}
