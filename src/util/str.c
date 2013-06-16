#include <stdlib.h> /* NULL */
#include <ctype.h>

#include "str.h"

char *str_quotefin(char *s)
{
	for(; *s; s++) switch(*s){
		case '\\':
			s++;
			break;
		case '"':
			return s;
	}

	return NULL;
}

char *str_spc_skip(char *s)
{
	for(; isspace(*s); s++);
	return s;
}
