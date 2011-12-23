#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "str.h"
#include "../util/alloc.h"

char *strreplace(char *line, const char *find, const char *replace)
{
	const unsigned int find_len    = strlen(find);
	const unsigned int replace_len = strlen(replace);
	unsigned int last;
	char *ret = ustrdup(line);
	char *pos;

	last = 0;

	while(last < strlen(ret) && (pos = strstr(ret + last, find))){
		char *del = ret;

		last = pos - ret + replace_len;
		*pos = '\0';

		ret = ustrprintf("%s%s%s", ret, replace, pos + find_len);

		free(del);
	}

	return ret ? ret : ustrdup(line);
}

int iswordpart(char c)
{
	return isalnum(c) || c == '_';
}

char *findword(char *line, char *word)
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
