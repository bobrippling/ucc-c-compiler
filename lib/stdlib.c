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

#define PAGE_SIZE 4096
/* getpagesize() */

#ifdef __DARWIN__
#  define MAP_ANONYMOUS MAP_ANON
#else
#  define MALLOC_SBRK
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

void *malloc(size_t size)
{
	/*
	 * TODO: linked list of free blocks, etc
	 */
#ifdef MALLOC_MMAP
	static void   *last_page     = NULL;
	static size_t  last_page_use = 0;

	void *p;
	int need_new;

	assert(size <= PAGE_SIZE); // TODO

	need_new = !last_page || last_page_use + size > PAGE_SIZE;

	// TODO: split this logic into malloc_chunk() and there is where we use sbrk() etc

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

#ifdef __DARWIN__
# error sbrk not implemented on darwin
#endif

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

void *realloc(void *p __unused, size_t l __unused)
{
	const char *s = "realloc() not implemented\n";
	write(2, s, strlen(s));
	abort();
}

void free(void *p __unused)
{
	/* no op... :C */
}

void abort()
{
	// TODO: unblock SIGABRT
	raise(SIGABRT);

	// TODO: restore + unblock SIGABRT and re-raise

	__builtin_unreachable();
}

char *getenv(const char *key)
{
	const size_t keylen = strlen(key);
	char **i;

	for(i = environ; *i; i++){
		char *equ, *e;

		e = *i;

		if((equ = strchr(e, '='))){
			const size_t len = equ - e;

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
	/* XXX: block cast to function pointer will
	 * break when lambdas are properly implemented
	 */
	return atexit((void (*)(void))f);
}

void exit(int code)
{
	/* XXX: stdio cleanup will go here */

	do_exit_funcs(&exit_funcs);
	_Exit(code);
}

void _Exit(int code)
{
	_exit(code);
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
