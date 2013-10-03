#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "str.h"
#include "macros.h"

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

static int overflow_chk(intval_t *const pv, int base, int inc)
{
	const intval_t v = *pv;

	*pv = v * base + inc;

	/* unsigned overflow is well defined
	 * if we're adding zero, ignore, e.g. a bare 0
	 * unless v > 0, in which case we need to check v*base didn't of
	 */
	if((inc > 0 || v > 0) && *pv <= v)
		return 1;

	return 0;
}

typedef int digit_test(int);

static char *overflow_handle(intval *pv, char *s, digit_test *test)
{
	warn_at(NULL, "overflow parsing integer, truncating to unsigned long long");

	while(test(*s))
		s++;

	/* force unsigned long long ULLONG_MAX */
	pv->val = INTVAL_T_MAX;
	pv->suffix = VAL_LLONG | VAL_UNSIGNED;
	return s;
}

static char *read_ap_num(digit_test test, char *s, intval *pval, int base)
{
	while(test(*s)){
		int dv = isdigit(*s) ? *s - '0' : tolower(*s) - 'a' + 10;
		if(overflow_chk(&pval->val, base, dv)){
			/* advance over what's left, etc */
			s = overflow_handle(pval, s, test);
			break;
		}
		s++;
		while(*s == '_')
			s++;
	}
	return s;
}

static int isbdigit(int c)
{
	return c == '0' || c == '1';
}

static int isodigit(int c)
{
	return '0' <= c && c < '8';
}

void char_seq_to_iv(char *s, intval *iv, int *plen, enum base mode)
{
	char *const start = s;
	struct
	{
		enum intval_suffix suff;
		int base;
		digit_test *test;
	} bases[] = {
		{ VAL_BIN,     2, isbdigit },
		{ VAL_OCTAL, 010, isodigit },
		{ 0,          10, isdigit  },
		{ VAL_HEX,  0x10, isxdigit },
	};

	memset(iv, 0, sizeof *iv);

	iv->suffix = bases[mode].suff;
	s = read_ap_num(bases[mode].test, s, iv, bases[mode].base);

	if(s == start)
		die_at(NULL, "invalid number (read 0 chars, at \"%s\")", s);

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

static int escape_multi_char(char *pos, char *pval, int *len)
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
					die_at(NULL, "unknown escape char '\\%c'", old_str[i]);
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
