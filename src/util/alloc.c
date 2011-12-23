#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../util/util.h"
#include "alloc.h"

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
