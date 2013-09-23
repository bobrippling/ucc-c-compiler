#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/escape.h"
#include "data_structs.h"
#include "str.h"
#include "macros.h"

void char_seq_to_iv(char *s, intval *iv, int *plen, enum base mode)
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

	char *const start = s;
	long lval = 0;

	memset(iv, 0, sizeof *iv);

	switch(mode){
		case BIN:
			iv->suffix = VAL_BIN;
			READ_NUM(*s == '0' || *s == '1', 2);
			break;

		case DEC:
			READ_NUM(isdigit(*s), 10);
			break;

		case OCT:
			iv->suffix = VAL_OCTAL;
			READ_NUM(isoct(*s), 010);
			break;

		case HEX:
		{
			int charsread = 0;
			iv->suffix = VAL_HEX;
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
				DIE_AT(NULL, "invalid hex char (read 0 chars, at \"%s\")", s);
			break;
		}
	}

	iv->val = lval;
	*plen = s - start;
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

int escape_multi_char(char *pos, char *pval, int *len)
{
	/* either \x or \[1-7][0-7]* */
	int is_oct;

	if((is_oct = ('0' <= *pos && *pos <= '7')) || *pos == 'x'){
		/* can't do '\b' - already defined */
		intval iv;

		if(is_oct){
			/*
			 * special case: '\0' is not oct
			 */
			if(*pos == '0' && !isoct(pos[1]))
				return 0;

			/* else got an oct, continuez vous */
		}else{
			pos++; /* skip the 'x' */
		}

		char_seq_to_iv(pos, &iv, len, is_oct ? OCT : HEX);

		*pval = iv.val;

		if(!is_oct)
			++*len; /* we stepped over the 'x' */

		return 1;
	}

	return 0;
}

void escape_string(char *old_str, int *plen)
{
	const int old_len = *plen;
	char *new_str = umalloc(*plen);
	int new_i, i;

	/* "parse" into another string */

	for(i = new_i = 0; i < old_len; i++){
		char add;

		if(old_str[i] == '\\'){
			int len;

			i++;

			if(escape_multi_char(old_str + i, &add, &len)){
				i += len - 1;
			}else{
				add = escape_char(old_str[i]);

				if(add == -1)
					DIE_AT(NULL, "unknown escape char '\\%c'", add);
			}
		}else{
			add = old_str[i];
		}

		new_str[new_i++] = add;
	}

	memcpy(old_str, new_str, new_i);
	*plen = new_i;
	free(new_str);
}

int literal_print(FILE *f, const char *s, int len)
{
	for(; len; s++, len--)
		if(*s == '\\'){
			if(fputs("\\\\", f) == EOF)
				return EOF;
		}else if(!isprint(*s)){
			if(fprintf(f, "\\%03o", *s) < 0)
				return EOF;
		}else if(fputc(*s, f) == EOF){
			return EOF;
		}

	return 0;
}
