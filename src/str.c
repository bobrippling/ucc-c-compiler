#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

int escapestring(char *str, int *len)
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
				extern int currentline, currentchar;
				extern const char *currentfname;

				fprintf(stderr,
						"%s:%d:%d Warning: Ignoring escape char before '%c'\n",
						currentfname, currentline, currentchar + 1, *c);
				memmove(c - 1, c, strlen(c) + 1);
			}
			--*len;
		}

	return 1;
}
