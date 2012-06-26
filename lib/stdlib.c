#include "stdlib.h"
#include "unistd.h"
#include "syscalls.h"
#include "signal.h"
#include "string.h"
#include "assert.h"
#include "errno.h"
#include "ucc_attr.h"

#include "sys/types.h"
#include "sys/mman.h"

#ifdef __DARWIN__
#  define MAP_ANONYMOUS MAP_ANON
#endif

int atoi(const char *s)
{
	int i = 0;

	while(*s)
		if('0' <= *s && *s <= '9')
			i = 10 * i + *s++ - '0';
		else
			break;

	return i;
}

void *calloc(size_t count, size_t len)
{
	const size_t sz = count * len;
	void *const p = malloc(sz); // overflow if too large..?

	if(!p)
		return NULL;

	memset(p, 0, sz);
	return p;
}

void *realloc(void *p __unused, size_t l __unused)
{
	const char *s = "realloc() not implemented\n";
	write(2, s, strlen(s));
	abort();
}

void abort()
{
	// TODO: unblock SIGABRT
	raise(SIGABRT);

	// TODO: restore + unblock SIGABRT and re-raise
}

char *getenv(const char *key)
{
	const int keylen = strlen(key);
	char **i;

	for(i = environ; *i; i++){
		char *equ, *e;

		e = *i;

		if((equ = strchr(e, '='))){
			const int len = equ - e;

			if(len == keylen && !strncmp(key, e, len))
				return equ + 1;
		}
	}

	return NULL;
}

#define N_EXITS 32

static struct exit_func
{
	void (*funcs[N_EXITS])(void);
	int    fidx;
} exit_funcs, quick_exit_funcs;


static void do_exit_funcs(struct exit_func *fs)
{
	/* call exit funcs */
	while(fs->fidx > 0)
		fs->funcs[--fs->fidx]();
}

static int add_exit_func(struct exit_func *to, void (*f)(void))
{
	if(to->fidx == N_EXITS){
		errno = ENOMEM;
		return -1;
	}

	to->funcs[to->fidx++] = f;
	return 0;
}

int atexit(void (*f)(void))
{
	return add_exit_func(&exit_funcs, f);
}

int atexit_b(void (^f)(void))
{
	return atexit(f);
}

void exit(int code)
{
	/* XXX: stdio cleanup will go here */

	do_exit_funcs(&exit_funcs);
	_Exit(code);
}

void _Exit(int code)
{
	__syscall(SYS_exit, code);
}

int at_quick_exit(void (*f)(void))
{
	return add_exit_func(&quick_exit_funcs, f);
}

void quick_exit(int code)
{
	/* XXX: stdio cleanup will go here */
	do_exit_funcs(&quick_exit_funcs);
	_Exit(code);
}
