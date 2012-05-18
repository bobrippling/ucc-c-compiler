#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "macro.h"
#include "parse.h"
#include "main.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

int current_line, current_chr;
int strip_in_block = 0;

struct
{
	FILE *file;
	char *fname;
	int line_no;
} file_stack[8] = { { NULL, NULL, 0 } };

int file_stack_idx = -1;

void preproc_out_info(void)
{
	/* output PP info */
	if(option_line_info)
		printf("# %d \"%s\"\n", file_stack[file_stack_idx].line_no, file_stack[file_stack_idx].fname);
}

void preproc_push(FILE *f, const char *fname)
{
	if(file_stack_idx >= 0)
		file_stack[file_stack_idx].line_no = current_line; /* save state */


	file_stack_idx++;
	if(file_stack_idx == ARRAY_LEN(file_stack))
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


	/* setup new state */
	file_stack[file_stack_idx].file    = f;
	file_stack[file_stack_idx].fname   = ustrdup(fname);
	file_stack[file_stack_idx].line_no = current_line = 1;

	preproc_out_info();
}

void preproc_pop(void)
{
	if(!file_stack_idx)
		ICE("file stack idx = 0 on pop()");

	free(file_stack[file_stack_idx].fname);

	file_stack_idx--;

#ifdef DO_CHDIR
	if(curwdfd != -1){
		if(fchdir(curwdfd) == -1)
			ppdie(p, "chdir(-): %s", strerror(errno));
		close(curwdfd);
	}
#endif

	/* restore state */
	current_line  = file_stack[file_stack_idx].line_no - 1;
	current_fname = file_stack[file_stack_idx].fname;

	preproc_out_info();
}

char *splice_line(void)
{
	static int n_nls;
	char *last;
	int join;

	if(n_nls){
		n_nls--;
		return ustrdup("");
	}

	last = NULL;
	join = 0;

	for(;;){
		FILE *f;
		int len;
		char *line;

re_read:
		if(file_stack_idx < 0)
			ICE("file stack idx = 0 on read()");
		f = file_stack[file_stack_idx].file;
		line = fline(f);

		if(!line){
			if(ferror(f))
				die("read():");

			fclose(f);
			if(file_stack_idx > 0){
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
			n_nls++;
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
		free(line);
		return NULL;
	}else{
		filter_macro(&line);
		return line;
	}
}

void preprocess()
{
	char *line;

	preproc_push(stdin, current_fname);

	while((line = splice_line())){
		char *s = filter_macros(strip_comment(line));
		if(s){
			puts(s);
			free(s);
		}
	}

	if(strip_in_block)
		die("no terminating block comment");

	macro_finish();
}
