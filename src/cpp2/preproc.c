#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "macro.h"

int current_line, current_chr;

char **splice_lines()
{
	char **lines;
	char *last;
	int join;

	lines = NULL;
	join  = 0;

	for(;;){
		int len;
		char *line;

		line = fline(stdin);

		if(!line){
			if(feof(stdin)){
				if(!lines)
					dynarray_add((void ***)&lines, ustrdup(""));
				return lines;
			}
			die("read():");
		}

		current_line++;

		if(join){
			join = 0;

			last = urealloc(last, strlen(last) + strlen(line) + 1);
			strcpy(last + strlen(last), line);
			free(line);
			line = last;
		}

		len = strlen(line);
		if(line[len - 1] == '\\'){
			line[len - 1] = '\0';
			join = 1;
			last = line;
		}else{
			dynarray_add((void ***)&lines, line);
		}
	}
}

char **strip_comments(char **lines)
{
	char **iter;
	int in_block;

	in_block = 0;

	for(iter = lines; *iter; iter++){
		char *s;
		for(s = *iter; *s; s++){
			if(in_block){
				if(*s == '*' && s[1] == '/'){
					*s = s[1] = ' ';
					s++;
					in_block = 0;
				}else{
					*s = ' ';
				}
			}else if(*s == '"'){
				/* read until the end of the string */
keep_going:
				s++;
				while(*s && *s != '"')
					s++;
				if(!*s)
					die("no terminating quote to comment");
				if(s[-1] == '\\')
					goto keep_going;
				/* finish of string */
			}else if(*s == '/'){
				if(s[1] == '/'){
					/* ignore //.* */
					*s = '\0';
					break;
				}else if(s[1] == '*'){
					/* wait for terminator elsewhere (i'll be back) */
					*s = s[1] = ' ';
					in_block = 1;
					s++;
				}
			}
		}
	}

	if(in_block)
		die("no terminating comment block");

	return lines;
}

char **filter_macros(char **lines)
{
	int i;

	for(i = 0; lines[i]; i++)
		if(*lines[i] == '#')
			handle_macro(lines + i);
		else
			filter_macro(lines + i);

	return lines;
}

char **output(char **lines)
{
	char **iter;
	for(iter = lines; *iter; iter++)
		printf("%s\n", *iter);
	return lines;
}

void preprocess()
{
	char **lines = output(filter_macros(strip_comments(splice_lines())));
	dynarray_free((void ***)&lines, free);
}
