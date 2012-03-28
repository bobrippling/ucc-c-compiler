#include "stdlib.h"
#include "unistd.h"
#include "syscalls.h"
#include "signal.h"
#include "string.h"
#include "assert.h"
#include "errno.h"

#include "sys/types.h"
#include "sys/mman.h"

#define PAGE_SIZE 4096
// getpagesize()

#ifdef __DARWIN__
# define MAP_ANONYMOUS MAP_ANON
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

void *malloc(size_t size)
{
#ifdef MALLOC_MMAP
	static void   *last_page     = NULL;
	static size_t  last_page_use = 0;

	void *p;
	int need_new;

	assert(size <= PAGE_SIZE); // TODO

	need_new = !last_page || last_page_use + size > PAGE_SIZE;

	if(need_new){
		last_page_use = 0;

		// memleak
		last_page = p = mmap(NULL, PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if(p == MAP_FAILED)
			return NULL;

	}else{
		p = last_page + last_page_use;
	}

	last_page_use += size;

	return p;
#else
# ifdef MALLOC_SBRK
	return sbrk(size);
# else
# warning malloc static buf implementation
	static char malloc_buf[PAGE_SIZE];
	static void *ptr = malloc_buf;
	void *ret = ptr;

	ptr += size;

	return ret; // TODO
# endif
#endif
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

void *realloc(void *p, size_t l)
{
	const char *s = "realloc() not implemented\n";
	write(2, s, strlen(s));
	abort();
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

#define ATEXIT_SINGLE

#ifdef ATEXIT_SINGLE
static void (*exit_func)(void);

#else
#define N_EXITS 32
#warning broken atexit()

static void (*exit_funcs[N_EXITS])(void);
static int    exit_fidx;
#endif

int atexit(void (*f)(void))
{
#ifdef ATEXIT_SINGLE
	if(exit_func){
		errno = ENOMEM;
		return -1;
	}
	exit_func = f;
	return 0;
#else
	if(exit_fidx == N_EXITS){
		errno = ENOMEM;
		return -1;
	}

	exit_funcs[exit_fidx++] = f;
	return 0;
#endif
}

void exit(int code)
{
	/* call exit funcs */
#ifdef ATEXIT_SINGLE
	if(exit_func)
		exit_func();
#else
	while(exit_fidx > 0)
		exit_funcs[--exit_fidx]();
#endif

	__syscall(SYS_exit, code);
}
