#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "str.h"

int escape_char(int c)
{
	struct
	{
		char from, to;
	} escapechars[] = {
		{ 'n', '\n'  },
		{ 't', '\t'  },
		{ 'b', '\b'  },
		{ 'r', '\r'  },
		{ 'v', '\v'  },
		{ 'f', '\f'  },
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

void char_seq_to_iv(char *s, intval *iv, int *plen, enum base mode)
{
#define READ_NUM(test, base)             \
			do{                                \
				if(!test)                        \
					break;                         \
				lval = base * lval + *s - '0';   \
				s++;                             \
				while(*s == '_')                 \
					s++;                           \
			}while(1)

	char *const start = s;
	long lval = 0;

	switch(mode){
		case BIN:
			READ_NUM(*s == '0' || *s == '1', 2);
			break;

		case DEC:
			READ_NUM(isdigit(*s), 10);
			break;

		case OCT:
			READ_NUM(isoct(*s), 010);
			break;

		case HEX:
		{
			int charsread = 0;
			do{
				if(isxdigit(*s)){
					lval = 0x10 * lval + (isdigit(tolower(*s)) ? *s - '0' : 10 + tolower(*s) - 'a');
					s++;
				}else{
					break;
				}
				charsread++;
				while(*s == '_')
					s++;
			}while(1);

			if(charsread < 1)
				die_at(NULL, "invalid hex char (read 0 chars, at \"%s\")", s);
			break;
		}
	}

	iv->val = lval;
	*plen = s - start;
}

int escape_multi_char(char **ppos)
{
	/* either \x or \[1-7][0-7]* */
	char *pos;
	int is_oct;

	pos = *ppos;

	if((is_oct = ('1' <= *pos && *pos <= '7')) || *pos == 'x'){
		/* TODO: binary */
		int len;
		intval iv;

		if(!is_oct)
			pos++;

		char_seq_to_iv(pos, &iv, &len, is_oct ? OCT : HEX);

		pos[-len + 1] = iv.val;

		memmove(pos + 1, pos + len, strlen(pos + 1 + len) + 1);

		*ppos = pos;

		return 1;
	}

	return 0;
}

void escape_string(char *str, int *len)
{
	char *c;
	int esc;

	for(c = str; *c; c++)
		if(*c == '\\'){
			char *const orig = ++c;

			//fprintf(stderr, "pre: \"%s\"\e[m\n", orig - 1);

			if(escape_multi_char(&c)){
				*len -= c - orig;
				memmove(orig, orig + 1, strlen(orig) + 1);
			}else{
				esc = escape_char(*c);

				if(esc == -1)
					die_at(NULL, "unknown escape char before '%c'", *c);

				orig[-1] = esc;
				memmove(orig, orig + 1, strlen(orig) + 1);
				--*len;
			}

			//fprintf(stderr, "pos: \"%s\"\e[m\n", orig - 1);
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
