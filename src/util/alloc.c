#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "../util/util.h"
#include "alloc.h"

void *umalloc(size_t l)
{
	void *p = calloc(1, l);
	if(!p)
		die("calloc %ld bytes:", l);
	return p;
}

void *urealloc(void *p, size_t l)
{
	void *r = realloc(p, l);
	if(!r)
		die("realloc %p by %ld bytes:", p, l);
	return r;
}

char *ustrdup(const char *s)
{
	char *r = umalloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

char *ustrdup2(const char *a, const char *b)
{
	int len = b - a;
	char *ret;
	assert(b >= a);
	ret = umalloc(len + 1);
	strncpy(ret, a, len);
	return ret;
}

char *ustrprintf(const char *fmt, ...)
{
	va_list l;
	char *buf = NULL;
	int len = 8, ret;

	do{
		len *= 2;
		buf = urealloc(buf, len);
		va_start(l, fmt);
		ret = vsnprintf(buf, len, fmt, l);
		va_end(l);
	}while(ret >= len);

	return buf;
}
