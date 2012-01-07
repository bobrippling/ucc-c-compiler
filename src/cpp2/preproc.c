#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "macro.h"

int current_line, current_chr;
int strip_in_block = 0;

FILE *file_stack[8] = { NULL };
int   file_stack_idx = 0;

void preproc_push(FILE *f)
{
	file_stack[file_stack_idx++] = f;
	if(file_stack_idx == sizeof(file_stack) / sizeof(file_stack[0]))
		die("too many includes");

#ifdef DO_CHDIR
		char *wd;

		curwdfd = open(".", O_RDONLY);
		if(curwdfd == -1)
			ppdie(p, "open(\".\"): %s", strerror(errno));

		/* make sure everything is relative to the file */
		wd = udirname(p->fname);
		if(chdir(wd))
			ppdie(p, "chdir(\"%s\"): %s (for %s)", wd, strerror(errno), p->fname);
		free(wd);
#endif
}

void preproc_pop(void)
{
	if(!file_stack_idx)
		ICE("file stack idx = 0 on pop()");
	file_stack_idx--;

#ifdef DO_CHDIR
	if(curwdfd != -1){
		if(fchdir(curwdfd) == -1)
			ppdie(p, "chdir(-): %s", strerror(errno));
		close(curwdfd);
	}
#endif
}

char *splice_line(void)
{
	char *last;
	int join;

	last = NULL;
	join = 0;

	for(;;){
		FILE *f;
		int len;
		char *line;

re_read:
		if(file_stack_idx <= 0)
			ICE("file stack idx = 0 on read()");
		f = file_stack[file_stack_idx - 1];
		line = fline(f);

		if(!line){
			if(ferror(f))
				die("read():");

			fclose(f);
			if(file_stack_idx > 1){
				free(dirname_pop());
				preproc_pop();
				goto re_read;
			}

			return NULL;
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
		if(len && line[len - 1] == '\\'){
			line[len - 1] = '\0';
			join = 1;
			last = line;
		}else{
			return line;
		}
	}
}

char *strip_comment(char *line)
{
	char *s;

	for(s = line; *s; s++){
		if(strip_in_block){
			if(*s == '*' && s[1] == '/'){
				*s = s[1] = ' ';
				s++;
				strip_in_block = 0;
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
				strip_in_block = 1;
				s++;
			}
		}
	}

	return line;
}

char *filter_macros(char *line)
{
	if(*line == '#'){
		handle_macro(line);
		*line = '\0';
	}else{
		filter_macro(&line);
	}

	return line;
}

char *output(char *line)
{
	printf("%s\n", line);
	return line;
}

void preprocess()
{
	char *line;

	preproc_push(stdin);

	while((line = splice_line()))
		free(output(filter_macros(strip_comment(line))));

	if(strip_in_block)
		die("no terminating block comment");

	macro_finish();
}
