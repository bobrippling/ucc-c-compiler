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

void *ucalloc(size_t n, size_t sz)
{
	void *p = calloc(n, sz);
	if(!p)
		ICE("calloc(%ld, %ld):", n, sz);
	return p;
}

void *urealloc(void *p, size_t new, size_t old)
{
	void *r = realloc(p, new);
	size_t diff = new - old;

	if(!r)
		ICE("realloc %p by %ld bytes:", p, (long)diff);

	/* if grown, zero the new space */
	if(diff > 0)
		memset((char *)r + old, 0, diff);

	return r;
}

void *urealloc1(void *p, size_t l)
{
	char *r = realloc(p, l);
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

char *ustrdup_or_null(const char *s)
{
	return s ? ustrdup(s) : NULL;
}

char *ustrdup2(const char *a, const char *b)
{
	int len = b - a;
	char *ret;
	assert(b >= a);
	ret = umalloc(len + 1);
	strncpy(ret, a, len);
	/* no need to nul-terminate,
	 * we have zeroed memory from umalloc up to len+1,
	 * and we copy up to len */
	return ret;
}

char *ustrvprintf(const char *fmt, va_list l)
{
	char *buf = NULL;
	int len = 8, ret;

	do{
		va_list lcp;
		int old = len;

		len *= 2;
		buf = urealloc(buf, len, old);

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
