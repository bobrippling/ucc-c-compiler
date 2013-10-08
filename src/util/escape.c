#include <stdio.h>
#include <stdlib.h> /* NULL */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "escape.h"
#include "util.h"
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

long char_seq_to_long(char *s, char **eptr, enum base mode, int limit)
{
#define READ_NUM(test, base, limit)      \
			do{                                \
				if(!(test))                      \
					break;                         \
				lval = base * lval + *s - '0';   \
				s++;                             \
				while(*s == '_')                 \
					s++;                           \
				limit;                           \
			}while(1)

	long lval = 0;

	switch(mode){
		case BIN:
			READ_NUM(*s == '0' || *s == '1', 2,);
			break;

		case DEC:
			READ_NUM(isdigit(*s), 10,);
			break;

		case OCT:
		{
			char *const begin = s;
			/* limit for binary */
			READ_NUM(isoct(*s), 010,
					if(limit && s - begin == 3)
						break
				);
			break;
		}

		case HEX:
		{
			int charsread = 0;
			do{
				if(isxdigit(*s)){
					lval = 0x10 * lval
						+ (isdigit(tolower(*s))
								? *s - '0'
								: 10 + tolower(*s) - 'a');

					s++;
				}else{
					break;
				}
				charsread++;
				while(*s == '_')
					s++;
			}while(1);

			if(charsread < 1){
				*eptr = NULL;
				return 0;
			}
			break;
		}
	}

	*eptr = s;
	return lval;
}

long read_char_single(char *start, char **end)
{
	long c = *start++;

	if(c == '\\'){
		char esc = tolower(*start);

		if(esc == 'x' || esc == 'b' || isoct(esc)){

			if(esc == 'x' || esc == 'b')
				start++;

			return char_seq_to_long(
					start, end,
					esc == 'x' ? HEX : esc == 'b' ? BIN : OCT, 1);

		}else{
			/* special parsing */
			c = escape_char(esc);

			if(c == -1){
				WARN_AT(NULL, "unrecognised escape character '%c'", esc);
				c = esc;
			}

			*end = start + 1;
		}
	}else{
		*end = start;
	}

	return c;
}

long read_quoted_char(
		char *start, char **end,
		int *multichar)
{
	unsigned long total = 0;

	*multichar = 0;

	if(*start == '\'') /* '' */
		DIE_AT(NULL, "empty char constant");

	for(;;){
		int ch;

		if(!*start)
			DIE_AT(NULL, "no terminating quote to character");

		ch = read_char_single(start, &start);
		total = (total * 256) + (0xff & ch);

		if(*start == '\'')
			break;


		*multichar = 1;
	}

	*end = start + 1;
	return total;
}

const char *base_to_str(enum base b)
{
	switch(b){
		case BIN: return "binary";
		case OCT: return "octal";
		case DEC: return "decimal";
		case HEX: return "hexadecimal";
	}
	return NULL;
}
