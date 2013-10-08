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

void escape_string(char *old_str, int *plen)
{
	char *new_str = umalloc(*plen);
	int i, new_i;

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
