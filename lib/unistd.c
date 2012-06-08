#include "unistd.h"
#include "syscalls.h"

#ifdef SYS_brk
static void *ucc_brk(void *p)
{
	return (void *)__syscall(SYS_brk, p);
}

int brk(void *p)
{
	void *ret;

	ret = ucc_brk(p);

	if(ret == p){
		return 0;
	}else{
		/* linux brk() returns the current break on failure */
		/*errno = ENOMEM;*/
		return -1;
	}
}

void *sbrk(int inc)
{
	void *new = ucc_brk(NULL);

	if(brk(new + inc) == -1){
		/*errno = ENOMEM;*/
		return (void *)-1;
	}

	return new;
}
#endif

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

int unlink(const char *f)
{
	return __syscall(SYS_unlink, f);
}

int rmdir(const char *d)
{
	return __syscall(SYS_rmdir, d);
}

int pipe(int fd[2])
{
	return __syscall(SYS_pipe, fd);
}

ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize)
{
	return __syscall(SYS_readlink, path, buf, bufsize);
}

int link(const char *from, const char *new)
{
	return __syscall(SYS_link, from, new);
}

int symlink(const char *link, const char *new)
{
	return __syscall(SYS_symlink, link, new);
}
