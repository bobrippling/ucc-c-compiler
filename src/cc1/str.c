#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/escape.h"
#include "data_structs.h"
#include "str.h"
#include "macros.h"

void escape_string(char *old_str, size_t *plen)
{
	char *new_str = umalloc(*plen);
	size_t i, new_i;

	/* "parse" into another string */

	for(i = new_i = 0; i < *plen; i++){
		char add;

		if(old_str[i] == '\\'){
			char *end;

      add = read_char_single(old_str + i, &end);

			UCC_ASSERT(end, "bad escape?");

			i = (end - old_str) - 1;

		}else{
			add = old_str[i];
		}

		new_str[new_i++] = add;
	}

	memcpy(old_str, new_str, new_i);
	*plen = new_i;
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
			int n = sprintf(p, "\\%03o", s[i]);
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
