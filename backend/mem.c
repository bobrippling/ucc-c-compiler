#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mem.h"

void *xcalloc(size_t n, size_t sz)
{
	void *p = calloc(n, sz);
	assert(p);
	return p;
}

void *xmalloc(size_t l)
{
	void *p = malloc(l);
	assert(p);
	return p;
}

char *xstrdup(const char *s)
{
	size_t l = strlen(s) + 1;
	char *new = xmalloc(l);
	memcpy(new, s, l);
	return new;
}
