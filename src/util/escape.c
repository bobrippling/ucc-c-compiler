#include <stdio.h>
#include <stdlib.h> /* NULL */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "escape.h"
#include "util.h"

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

long escape_multi_char(char *pos, char **eptr)
{
	int is_oct;

	/* either \x or \[1-7][0-7]* */
	if((is_oct = ('0' <= *pos && *pos <= '7')) || *pos == 'x'){
		if(is_oct){
			/*
			 * special case: '\0' is not oct
			 */
			if(*pos == '0' && !isoct(pos[1])){
				*eptr = pos + 1;
				return 0;
			}

			/* else got an oct, continuez vous */
		}else{
			pos++; /* skip the 'x' */
		}

		return char_seq_to_long(pos, eptr, is_oct ? OCT : HEX);
	}

	*eptr = NULL;
	return -1;
}

long char_seq_to_long(char *s, char **eptr, enum base mode)
{
#define READ_NUM(test, base)             \
			do{                                \
				if(!(test))                      \
					break;                         \
				lval = base * lval + *s - '0';   \
				s++;                             \
				while(*s == '_')                 \
					s++;                           \
			}while(1)

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
