#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "str.h"
#include "../util/alloc.h"

static int iswordpart(char c)
{
	return isalnum(c) || c == '_';
}

char *word_replace(char *line, char *pos, const char *find, const char *replace)
{
	const unsigned int len_find    = strlen(find);
	const unsigned int len_replace = strlen(replace);

	if(len_replace < len_find){
		/* duh */
		memcpy(pos, replace, len_replace);
		memmove(pos + len_replace, pos + len_find, strlen(pos + len_find) + 1);
		return line;
	}else{
		char *del = line;

		*pos = '\0';
		line = ustrprintf("%s%s%s", line, replace, pos + len_find);
		free(del);

		return line;
	}
}

char *word_find(char *line, char *word)
{
	const int wordlen = strlen(word);
	char *pos = line;

	while((pos = strstr(pos, word))){
		char *fin;
		if(pos > line && iswordpart(pos[-1])){
			pos++;
			continue;
		}
		fin = pos + wordlen;
		if(iswordpart(*fin)){
			pos++;
			continue;
		}
		return pos;
	}
	return NULL;
}
