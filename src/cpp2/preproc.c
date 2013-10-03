#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/str.h"

#include "main.h"
#include "preproc.h"
#include "directive.h"
#include "eval.h"
#include "str.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

static int strip_in_block = 0;

struct
{
	FILE *file;
	char *fname;
	int line_no;
} file_stack[8] = { { NULL, NULL, 0 } };

int file_stack_idx = -1;

void include_bt(FILE *f)
{
	int i;

	for(i = 0; i < file_stack_idx; i++){
		fprintf(f, "%sfrom: %s:%d\n",
				i == 0 ?
				"in file included " :
				"                 ",
				file_stack[i].fname,
				file_stack[i].line_no - 1);
	}
}

void preproc_backtrace()
{
	include_bt(stderr);
}

static void preproc_out_info(void)
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
		CPP_DIE("too many includes");

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

static void preproc_pop(void)
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
	current_line  = file_stack[file_stack_idx].line_no;
	current_fname = file_stack[file_stack_idx].fname;

	preproc_out_info();
}

static char *read_line(void)
{
	FILE *f;
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

	return line;
}

static char *expand_trigraphs(char *line)
{
	static const struct
	{
		char from, to;
	} map[] = {
		{ '(', '[' },
		{ ')', ']' },
		{ '<', '{' },
		{ '>', '}' },
		{ '=', '#' },
		{ '/', '\\' },
		{ '\'', '^' },
		{ '!', '|' },
		{ '-', '~' },
		{ 0, 0 }
	};
	int i;

	for(;;){
		char *qmark = strstr(line, "??");

		if(!qmark)
			break;

		for(i = 0; map[i].from; i++){
			if(map[i].from == qmark[2]){
				qmark[0] = map[i].to;
				memmove(qmark + 1, qmark + 3, strlen(qmark + 3) + 1);
				break;
			}
		}
	}

	return line;
}

static char *expand_digraphs(char *line)
{
	static const struct
	{
		const char *from;
		char to;
	} map[] = {
		{ "<:", '[' },
		{ ":>", ']' },
		{ "<%", '{' },
		{ "%>", '}' },
		{ "%:", '#' },
		{ 0, 0 }
	};
	int i;

	for(i = 0; map[i].from; i++){
		char *pos = line;
		while((pos = strstr(pos, map[i].from))){
			*pos = map[i].to;
			memmove(pos + 1, pos + 2, strlen(pos + 2) + 1);
		}
	}

	return line;
}

static char *splice_lines(int *peof)
{
	static int n_nls;
	char *line;
	size_t len;

	if(n_nls){
		n_nls--;
		return ustrdup("");
	}

	line = read_line();
	if(!line){
		*peof = 1;
		return NULL;
	}
	if(option_trigraphs)
		line = expand_trigraphs(line);

	len = strlen(line);

	if(len && line[len - 1] == '\\'){
		char *next = splice_lines(peof);

		/* remove in any case */
		line[len - 1] = '\0';

		if(next){
			const size_t next_len = strlen(next);

			n_nls++;

			line = urealloc1(line, len + next_len + 1);
			strcpy(line + len - 1, next);
			free(next);
		}else{
			/* backslash-newline at eof
			 * else we may return non-null on eof,
			 * so we need an eof-check in read_line()
			 */
			CPP_WARN(WFINALESCAPE, "backslash-escape at eof");
			*peof = 1;
		}
	}

	return line;
}

static char *strip_comment(char *line)
{
	const int is_directive = *str_spc_skip(line) == '#';
	char *s;

	for(s = line; *s; s++){
		if(strip_in_block){
			const int clear_comments = (strip_comments == STRIP_ALL);

			if(*s == '*' && s[1] == '/'){
				strip_in_block = 0;
				if(clear_comments)
					*s++ = ' ';
			}
			if(clear_comments)
				*s = ' ';

		}else if(*s == '"'){
			/* read until the end of the string */
			s = terminating_quote(s + 1);
			if(!s)
				CPP_DIE("no terminating quote to string");
			/* finish of string */
		}else if(*s == '/'){
			if(s[1] == '/'){
				switch(strip_comments){
					case STRIP_EXCEPT_DIRECTIVE:
						if(is_directive)
							goto strip_1line;
						/* fall */
					case STRIP_NONE:
					{
						/* convert to a block comment */
						const unsigned pos_slash = s - line;
						const unsigned len = strlen(line);

						line = urealloc1(line, len + 2 + 1);

						s = line + pos_slash;
						s[1] = '*';
						strcpy(line + len, "*/");
						break;
					}
strip_1line:
					case STRIP_ALL:
						/* ignore //.* */
						*s = '\0';
						break;
				}
				break;
			}else if(s[1] == '*'){
				strip_in_block = 1;

				switch(strip_comments){
					case STRIP_EXCEPT_DIRECTIVE:
						if(!is_directive)
							break;
						/* fall */
					case STRIP_ALL:
						/* wait for terminator elsewhere (i'll be back) */
						*s = s[1] = ' ';
						s++;
					case STRIP_NONE:
						break;
				}
			}
		}
	}

	return line;
}

static char *filter_macros(char *line)
{
	/* check for non-standard space-then-# */
	char *hash = line;

	if(*hash == '#' || *(hash = str_spc_skip(hash)) == '#'){
		if(*line != '#')
			CPP_WARN(WTRADITIONAL, "'#' for directive isn't in the first column");

		parse_directive(hash + 1);
		free(line);
		return NULL;
	}else{
		if(parse_should_noop())
			*line = '\0';
		else
			line = eval_expand_macros(line);
		return line;
	}
}

void preprocess(void)
{
	char *line;
	int eof = 0;

	preproc_push(stdin, current_fname);

	while(!eof && (line = splice_lines(&eof))){
		debug_push_line(line);

		if(option_digraphs)
			line = expand_digraphs(line);

		line = strip_comment(line);
		if(!strip_in_block)
			line = filter_macros(line);

		debug_pop_line();

		if(line){
			if(!no_output)
				puts(line);
			free(line);
		}
	}

	if(strip_in_block)
		CPP_DIE("no terminating block comment");

	parse_end_validate();
}
