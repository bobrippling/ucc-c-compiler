#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "../util/util.h"

int escapechar(int c)
{
	struct
	{
		char from, to;
	} escapechars[] = {
		{ 'n', '\n'  },
		{ 't', '\t'  },
		{ 'b', '\b'  },
		{ 'r', '\r'  },
		{ '0', '\0'  },
		{ 'e', '\33' },
		{ '\\', '\\' },
		{ '\'', '\'' },
		{ '"',  '"'  }
	};
	unsigned int i;

	for(i = 0; i < sizeof(escapechars) / sizeof(escapechars[0]); i++)
		if(escapechars[i].from == c)
			return escapechars[i].to;

	return -1;
}

void escapestring(char *str, int *len)
{
	char *c;
	int esc;

	for(c = str; *c; c++)
		if(*c == '\\'){
			if((esc = escapechar(*++c)) >= 0){
				c[-1] = esc;
				memmove(c, c+1, strlen(c)); /* strlen(c) to include \0 */
				c--;
			}else{
				warn_at(NULL, "ignoring escape char before '%c'", *c);
				memmove(c - 1, c, strlen(c) + 1);
			}
			--*len;
		}
}

int literalprint(FILE *f, const char *s, int len)
{
	for(; len; s++, len--)
		if(!isprint(*s)){
			if(fprintf(f, "\\%03o", *s) < 0)
				return EOF;
		}else if(fputc(*s, f) == EOF)
			return EOF;

	return 0;
}

