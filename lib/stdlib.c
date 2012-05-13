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

static void (*exit_funcs[N_EXITS])(void);
static int    exit_fidx;

int atexit(void (*f)(void))
{
	if(exit_fidx == N_EXITS){
		errno = ENOMEM;
		return -1;
	}

	exit_funcs[exit_fidx++] = f;
	return 0;
}

void exit(int code)
{
	/* call exit funcs */
	while(exit_fidx > 0)
		exit_funcs[--exit_fidx]();

	__syscall(SYS_exit, code);
}
