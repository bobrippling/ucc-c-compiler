#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "str.h"
#include "../util/alloc.h"
#include "../util/util.h"
#include "../util/str.h"
#include "macro.h"

int iswordpart(char c)
{
	return isalnum(c) || c == '_';
}

char *word_end(char *s)
{
	for(; iswordpart(*s); s++);
	return s;
}

char *word_find_any(char *s)
{
	char in_quote = 0;
	for(; *s; s++){
		switch(*s){
			case '"':
			case '\'':
				if(in_quote == *s)
					in_quote = 0;
				else
					in_quote = *s;
				break;

			case '\\':
				if(in_quote){
					if(!*++s)
						break;
					continue;
				}
		}
		if(!in_quote && iswordpart(*s))
			return s;
	}
	return NULL;
}

void str_trim(char *str)
{
	char *s = str_spc_skip(str);

	memmove(str, s, strlen(s) + 1);

	for(s = str; *s; s++);

	while(s > str && isspace(*s))
		s--;

	*s = '\0';
}

char *str_join(char **strs, const char *with)
{
	const int with_len = strlen(with);
	int len;
	char **itr;
	char *ret, *p;

	len = 1; /* \0 */
	for(itr = strs; *itr; itr++)
		len += strlen(*itr) + with_len;

	p = ret = umalloc(len);

	for(itr = strs; *itr; itr++)
		p += sprintf(p, "%s%s", *itr, itr[1] ? with : "");

	return ret;
}

char *word_dup(const char *s)
{
	const char *start = s;
	while(iswordpart(*s))
		s++;
	return ustrdup2(start, s);
}

char *str_quote(char *quoteme, int free_in)
{
	int len;
	const char *s;
	char *ret, *p;

	len = 3; /* ""\0 */
	for(s = quoteme; *s; s++, len++)
		switch(*s){
			case '"':
			case '\\':
				len++;
		}

	p = ret = umalloc(len);

	*p++ = '"';

	for(s = quoteme; *s; s++){
		switch(*s){
			case '"':
			case '\\':
				*p++ = '\\';
		}
		*p++ = *s;
	}

	strcpy(p, "\"");

	if(free_in)
		free(quoteme);
	return ret;
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
		line = ustrprintf("%s%s%s", line, replace, end);
		free(del);

		return line;
	}
}

char *word_replace(char *line, char *pos, size_t len, const char *replace)
{
	return str_replace(line, pos, pos + len, replace);
}

static char *word_strstr(char *haystack, char *needle)
{
	const int nlen = strlen(needle);
	char *i;

	if(!strstr(haystack, needle))
		return NULL;

	for(i = haystack; *i; i++)
		if(*i == '"'){
			i = str_quotefin(i + 1);
			if(!i)
				ICE("terminating quote not found\nhaystack = >>%s<<\nneedle = >>%s<<",
						haystack, needle);
		}else if(!strncmp(i, needle, nlen)){
			return i;
		}

	return NULL;
}

char *word_find(char *line, char *word)
{
	const int wordlen = strlen(word);
	char *pos = line;

	while((pos = word_strstr(pos, word))){
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

char *strchr_nest(char *start, char find)
{
	size_t nest = 0;

	for(; *start; start++)
		switch(*start){
			case '(':
				nest++;
				break;
			case ')':
				if(nest > 0)
					nest--;
				/* fall */
			default:
				if(nest == 0 && *start == find)
					return start;
		}

	return NULL;
}
