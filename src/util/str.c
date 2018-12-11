#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "str.h"

char *str_quotefin2(char *s, char q)
{
	for(; *s; s++)
		if(*s == '\\')
			s++;
		else if(*s == q)
			return s;

	return NULL;
}

char *char_quotefin(char *s)
{
	return str_quotefin2(s, '\'');
}

char *str_quotefin(char *s)
{
	return str_quotefin2(s, '"');
}

char *str_spc_skip(const char *s)
{
	for(; isspace(*s); s++);
	return (char *)s;
}

int str_endswith(const char *haystack, const char *needle)
{
	size_t h_l = strlen(haystack);
	size_t n_l = strlen(needle);

	if(n_l > h_l)
		return 0;

	return !strcmp(haystack + h_l - n_l, needle);
}

int xsnprintf(char *buf, size_t len, const char *fmt, ...)
{
	va_list l;
	int desired_space;

	va_start(l, fmt);
	desired_space = vsnprintf(buf, len, fmt, l);
	va_end(l);

	if(desired_space < 0 || desired_space >= (int)len){
		fprintf(stderr, "snprintf() overflow\n");
		abort();
	}

	return desired_space;
}
