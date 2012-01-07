#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "str.h"
#include "../util/alloc.h"
#include "macro.h"

static int iswordpart(char c)
{
	return isalnum(c) || c == '_';
}

char *str_replace(char *line, char *start, char *end, const char *replace)
{
	const unsigned int len_find    = end - start;
	const unsigned int len_replace = strlen(replace);

	if(len_replace < len_find){
		/* start<->end distance is less than replace.length */
		memcpy(start, replace, len_replace);
		memmove(start + len_replace, end, strlen(end) + 1);
		return line;
	}else{
		char *del = line;

		*start = '\0';
		line = ustrprintf("%s%s%s", line, replace, end + 1);
		free(del);

		return line;
	}
}

char *word_replace(char *line, char *pos, const char *find, const char *replace)
{
	return str_replace(line, pos, pos + strlen(find), replace);
}

int word_replace_g(char **pline, char *find, const char *replace)
{
	char *pos = *pline;
	int r = 0;

	DEBUG(DEBUG_VERB, "word_find(\"%s\", \"%s\")\n", pos, find);

	while((pos = word_find(pos, find))){
		int posidx = pos - *pline;

		DEBUG(DEBUG_VERB, "word_replace(line=\"%s\", pos=\"%s\", nam=\"%s\", val=\"%s\")\n",
				*pline, pos, find, replace);

		*pline = word_replace(*pline, pos, find, replace);
		pos = *pline + posidx + strlen(replace);

		r = 1;
	}

	return r;
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
