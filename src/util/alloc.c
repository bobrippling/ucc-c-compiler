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
		ICE("calloc %ld bytes:", (long)l);
	return p;
}

void *urealloc(void *p, size_t l)
{
	void *r = realloc(p, l);
	if(!r)
		ICE("realloc %p by %ld bytes:", p, (long)l);
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

char *ustrvprintf(const char *fmt, va_list l)
{
	char *buf = NULL;
	int len = 8, ret;

	do{
		va_list lcp;

		len *= 2;
		buf = urealloc(buf, len);

		va_copy(lcp, l);
		ret = vsnprintf(buf, len, fmt, lcp);
		va_end(lcp);

	}while(ret >= len);

	return buf;

}

char *ustrprintf(const char *fmt, ...)
{
	va_list l;
	char *r;
	va_start(l, fmt);
	r = ustrvprintf(fmt, l);
	va_end(l);
	return r;
}
