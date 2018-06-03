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

	*of = 0;

	while(test(*s)){
		int dv = isdigit(*s) ? *s - '0' : tolower(*s) - 'a' + 10;

		if(inc_and_chk(&val, base, dv)){
			/* advance over what's left, etc */
			overflow_handle(s, end, test);
			*of = 1;
			while(test(*s) || *s == '_')
				s++;
			break;
		}
		s++;
		while(*s == '_')
			s++;
		if(max_chars > 0 && --max_chars == 0)
			break;
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

long read_char_single(char *start, char **end, int *const warn)
{
	long c = *start++;

	*warn = 0;

	if(c == '\\'){
		char esc = tolower(*start);

		/* no binary here - only in numeric constants */
		if(esc == 'x' || isoct(esc)){
			long parsed;
			int of;

			if(esc == 'x' || esc == 'b')
				start++;

			parsed = char_seq_to_ullong(
					start,
					end,
					/*apply_limit*/1,
					esc == 'x' ? HEX : esc == 'b' ? BIN : OCT,
					&of);

			if((unsigned)parsed > 0xff)
				*warn = ERANGE;

			return parsed;

		}else{
			/* special parsing */
			c = escape_char(esc);

			if(c == -1)
				*warn = EINVAL;

			*end = start + 1;
		}
	}else{
		*end = start;
	}

	return c;
}

long read_quoted_char(
		char *start, char **end,
		int *multichar, int clip_256,
		const char **const err,
		int *const warn)
{
	unsigned long total = 0;
	unsigned i;

	*multichar = 0;
	*err = NULL;

	if(*start == '\''){
		/* '' */
		*err = "empty char constant";
		goto out;
	}

	for(i = 0;; i++){
		int ch;

		if(!*start){
			*err = "no terminating quote to character";
			goto out;
		}

		ch = read_char_single(start, &start, warn);

		if(clip_256)
			total = (total * 256) + (0xff & ch);
		else
			total += ch;

		if(*start == '\'')
			break;


		*multichar = 1;
	}

out:
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
