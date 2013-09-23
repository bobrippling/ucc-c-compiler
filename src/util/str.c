#include <stdlib.h> /* NULL */
#include <ctype.h>

#include "str.h"

static char *str_quotefin2(char *s, char q)
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

char *str_spc_skip(char *s)
{
	for(; isspace(*s); s++);
	return s;
}
