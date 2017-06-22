#include "unistd.h"
#include "syscalls.h"
#include "sys/time.h"
#include "sys/select.h"
#include "stdlib.h" /* __progname */

char **environ;
char *__progname;

#ifdef SYS_brk
static void *ucc_brk(void *p)
{
	return (void *)__syscall(SYS_brk, p);
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
	void *new = (char *)ucc_brk(NULL) + inc;

	if(brk(new) == -1){
		/*errno = ENOMEM;*/
		return (void *)-1;
	}

	return new;
}
#else
#  warning no brk() on darwin
#endif

unsigned int sleep(unsigned int sec)
{
	struct timespec tsp, rem;

	tsp.tv_sec = sec;
	tsp.tv_nsec = 0;

	nanosleep(&tsp, &rem);

	/* seconds left */
	return rem.tv_sec;
}

int usleep(useconds_t usec)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = usec;

	return select(0, NULL, NULL, NULL, &tv);
}

pid_t getpid(void)
{
	return __syscall(SYS_getpid);
}

int read(int fd, void *p, int size)
{
	return __syscall(SYS_read, fd, p, size);
}

int write(int fd, const void *p, int size)
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

int pipe(int fd[static 2])
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

off_t lseek(int fd, off_t offset, int whence)
{
	return (off_t)__syscall(SYS_lseek, fd, offset, whence);
}

void _exit(int code)
{
	__syscall(SYS_exit, code);
	__builtin_unreachable();
}
