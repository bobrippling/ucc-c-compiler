#include <stdio.h>
#include <stdlib.h> /* NULL */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "escape.h"
#include "str.h"
#include "macros.h"

typedef int digit_test(int);


static int escape_char1(int c)
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
		{ 'a', '\a'  },
		{ 'e', '\33' },
		{ '\\', '\\' },
		{ '\'', '\'' },
		{ '?',  '?'  }, /* for avoiding trigraphs */
		{ '"',  '"'  }
	};
	unsigned int i;

	for(i = 0; i < countof(escapechars); i++)
		if(escapechars[i].from == c)
			return escapechars[i].to;

	return -1;
}

static int inc_and_chk(unsigned long long *const val, unsigned base, unsigned inc)
{
	/* unsigned overflow is well defined */
	const unsigned long long new = *val * base + inc;

	/* can't just check: new < *val
	 * since if base=16, inc=15 (0xff),
	 * then: 0xffff..ff * 16 = 0xffff..00
	 *  and: + 0xff = 0xffff..ff
	 * and we start again.
	 */
	int of = new < *val || *val * base < *val;

	*val = new;

	return of;
}

static void overflow_handle(char *s, char **end, digit_test *test)
{
	while(test(*s))
		s++;

	*end = s;
}

static unsigned long long read_ap_num(
		digit_test test, char *s,
		int base, int max_chars,
		char **const end, int *const of)
{
	unsigned long long val = 0;
	int i = 0;

	*of = 0;

	while(test(*s)){
		int dv = isdigit(*s) ? *s - '0' : tolower(*s) - 'a' + 10;

		if(inc_and_chk(&val, base, dv)){
			/* advance over what's left, etc */
			overflow_handle(s, end, test);
			*of = 1;
			while((test(*s) || *s == '_'))
				s++;
			break;
		}
		s++;
		while(*s == '_')
			s++;
		if(max_chars > 0 && --max_chars == 0)
			break;

		i++;
	}

	*end = s;

	return val;
}

static int isbdigit(int c)
{
	return c == '0' || c == '1';
}

static int isodigit(int c)
{
	return '0' <= c && c < '8';
}

unsigned long long char_seq_to_ullong(
		char *s, char **const eptr, int apply_limit, enum base mode, int *const of)
{
	static const struct
	{
		int base;
		int max_chars;
		digit_test *test;
	} bases[] = {
		{    2, -1, isbdigit },
		{  010,  3, isodigit },
		{   10, -1, isdigit  },
		{ 0x10, -1, isxdigit },
	};

	return read_ap_num(
			bases[mode].test,
			s,
			bases[mode].base,
			apply_limit ? bases[mode].max_chars : -1,
			eptr,
			of);
}

int escape_char_1(char *start, char **const end, int *const warn, int one_byte_limit)
{
	/* no binary here - only in numeric constants */
	char esc = *start;

	if(esc == 'x' || isoct(esc)){
		unsigned long long parsed;
		int overflow;

		if(esc == 'x')
			start++;

		parsed = char_seq_to_ullong(
				start,
				end,
				one_byte_limit,
				esc == 'x' ? HEX : OCT,
				&overflow);

		if(overflow)
			*warn = ERANGE;
		else if(one_byte_limit && parsed > 0xffU)
			*warn = ERANGE;
		else if(!one_byte_limit && parsed > 0xffffffffU)
			*warn = ERANGE;

		return parsed;

	}else{
		/* special parsing */
		int c = escape_char1(esc);

		if(c == -1)
			*warn = EINVAL;

		*end = start + 1;

		return c;
	}
}

int escape_char(
		char *start,
		/*nullable*/char *limit,
		char **const end,
		int is_wide,
		int *const multi,
		int *const warn,
		int *const err)
{
	int ret = 0;
	char *i;
	size_t n = 0;

	*warn = *err = 0;

	/* assuming start..end doesn't contain nuls */
	for(i = start; i != limit && *i; i++){
		int this;

		if(*i == '\\'){
			char *escfin;

			i++;
			if(i == limit){
				*err = EILSEQ;
				break;
			}

			this = escape_char_1(i, &escfin, warn, /*limit-to-1-byte*/!is_wide);

			i = escfin /*for inc:*/- 1;
		}else{
			this = *i;
		}

		if(is_wide)
			ret = this; /* truncate to last parsed char */
		else
			ret = ret * 256 + this;

		n++;
	}

	if(multi)
		*multi = n > 1;

	if(i == limit)
		*end = i;
	else
		*end = i - 1;

	return ret;
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
