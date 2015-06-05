#include <stdio.h>
#include <stdlib.h> /* NULL */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "escape.h"
#include "util.h"
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
	warn_at(NULL, "overflow parsing integer, truncating to unsigned long long");

	while(test(*s))
		s++;

	*end = s;
}

static unsigned long long read_ap_num(
		digit_test test, char *s, int base,
		int max_n,
		char **end, int *of)
{
	unsigned long long val = 0;
	int i = 0;

	*of = 0;

	while(test(*s) && (max_n == -1 || i++ < max_n)){
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
		char *s, char **eptr, enum base mode,
		int limit, int *of)
{
	static const struct
	{
		int base, max_n;
		digit_test *test;
	} bases[] = {
		{    2, -1, isbdigit },
		{  010,  3, isodigit },
		{   10, -1, isdigit  },
		{ 0x10, -1, isxdigit },
	};
	unsigned long long val;

	val = read_ap_num(
			bases[mode].test, s, bases[mode].base,
			limit ? bases[mode].max_n : -1,
			eptr, of);

	if(s == *eptr)
		die_at(NULL, "invalid number (read 0 chars, at \"%s\")", s);

	return val;
}

long read_char_single(char *start, char **end, unsigned off)
{
	long c = *start++;

	if(c == '\\'){
		char esc = tolower(*start);

		/* no binary here - only in numeric constants */
		if(esc == 'x' || isoct(esc)){
			int of; /* XXX: overflow ignored */

			if(esc == 'x' || esc == 'b')
				start++;

			return char_seq_to_ullong(
					start, end,
					esc == 'x' ? HEX : esc == 'b' ? BIN : OCT,
					1, &of);

		}else{
			/* special parsing */
			c = escape_char(esc);

			if(c == -1){
				where loc;

				where_current(&loc);
				loc.chr += off + 1;
				loc.len = 2;

				warn_at(&loc, "unrecognised escape character '%c'", esc);
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
		int *multichar, int clip_256)
{
	unsigned long total = 0;
	unsigned i;

	*multichar = 0;

	if(*start == '\'') /* '' */
		die_at(NULL, "empty char constant");

	for(i = 0;; i++){
		int ch;

		if(!*start)
			die_at(NULL, "no terminating quote to character");

		ch = read_char_single(start, &start, i);

		if(clip_256)
			total = (total * 256) + (0xff & ch);
		else
			total += ch;

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
