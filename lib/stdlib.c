#include "stdlib.h"
#include "unistd.h"
#include "syscalls.h"
#include "signal.h"
#include "string.h"
#include "assert.h"

#include "sys/types.h"
#include "sys/mman.h"

#define PAGE_SIZE 4096
// getpagesize()

#define MALLOC_MMAP

#ifdef __DARWIN__
# define MAP_ANONYMOUS MAP_ANON
#endif

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
	static malloc_buf[PAGE_SIZE];
	return ...; // TODO
# endif
#endif
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
		char save, *equ, *e;

		e = *i;

		if(equ = strchr(e, '=')){
			const int len = equ - e;

			if(len == keylen && !strncmp(key, e, len))
				return equ + 1;
		}
	}

	return NULL;
}
