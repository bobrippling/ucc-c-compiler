#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "alloc.h"
#include "util.h"

void *umalloc(size_t l)
{
	void *p = malloc(l);
	if(!p)
		die("malloc %ld bytes:", l);
	memset(p, 0, l);
	return p;
}

void *urealloc(void *p, size_t l)
{
	void *r = realloc(p, l);
	if(!r)
		die("realloc %p by %ld bytes:", l);
	return r;
}

char *ustrdup(const char *s)
{
	char *r = umalloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}
