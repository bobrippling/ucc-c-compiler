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

/* --- */

int fork()
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
