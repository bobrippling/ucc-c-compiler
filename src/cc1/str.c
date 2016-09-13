#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/escape.h"
#include "str.h"
#include "macros.h"
#include "cc1_where.h"
#include "warn.h"
#include "parse_fold_error.h"

void escape_string(char *old_str, size_t *plen)
{
	char *const new_str = umalloc(*plen);
	size_t i, iout;

	/* "parse" into another string */

	for(i = iout = 0; i < *plen; i++){
		char add;

		if(old_str[i] == '\\'){
			where loc;
			char *end;
			int warn;

			where_cc1_current(&loc);
			loc.chr += i + 1;

			add = escape_char_1(&old_str[i + 1], &end, &warn, /*1bytelim*/1);

			UCC_ASSERT(end, "bad parse?");

			i = (end - old_str) /*for the loop inc:*/- 1;

			switch(warn){
				case 0:
					break;
				case ERANGE:
					warn_at_print_error(&loc,
							"escape sequence out of range (larger than 0xff)");
					break;
				case EINVAL:
					warn_at_print_error(&loc, "invalid escape character");
					parse_had_error = 1;
					break;
			}

		}else{
			add = old_str[i];
		}

		new_str[iout++] = add;
	}

	assert(iout <= *plen);

	memcpy(old_str, new_str, iout);
	*plen = iout;
	free(new_str);
}

char *str_add_escape(const char *s, const size_t len)
{
	size_t nlen = 0, i;
	char *new, *p;

	for(i = 0; i < len; i++)
		if(s[i] == '\\' || s[i] == '"')
			nlen += 3;
		else if(!isprint(s[i]))
			nlen += 4;
		else
			nlen++;

	p = new = umalloc(nlen + 1);

	for(i = 0; i < len; i++)
		if(s[i] == '\\' || s[i] == '"'){
			*p++ = '\\';
			*p++ = s[i];
		}else if(!isprint(s[i])){
			/* cast to unsigned char so we don't try printing
			 * some massive sign extended negative number */
			int n = sprintf(p, "\\%03o", (unsigned char)s[i]);
			UCC_ASSERT(n <= 4, "sprintf(octal), length %d > 4", n);
			p += n;
		}else{
			*p++ = s[i];
		}

	return new;
}

int literal_print(FILE *f, const char *s, size_t len)
{
	char *literal = str_add_escape(s, len);
	int r = fprintf(f, "%s", literal);
	free(literal);
	return r;
}
