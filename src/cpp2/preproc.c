#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/str.h"
#include "../util/macros.h"

#include "main.h"
#include "preproc.h"
#include "directive.h"
#include "eval.h"
#include "str.h"

static enum
{
	NOT_IN_BLOCK,
	IN_BLOCK_BEGIN,
	IN_BLOCK_END,
	IN_BLOCK_FULL
} strip_in_block = NOT_IN_BLOCK;

struct
{
	FILE *file;
	char *fname;
	int line_no;
} file_stack[64] = { { NULL, NULL, 0 } };

int file_stack_idx = -1;
static int prev_newline;

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

static void preproc_out_info(void)
{
	/* output PP info */
	if(!no_output && option_line_info)
		printf("# %d \"%s\"\n", file_stack[file_stack_idx].line_no, file_stack[file_stack_idx].fname);
}

int preproc_in_include()
{
	return file_stack_idx > 0;
}

void preproc_push(FILE *f, const char *fname)
{
	if(file_stack_idx >= 0)
		file_stack[file_stack_idx].line_no = current_line; /* save state */


	file_stack_idx++;
	if(file_stack_idx == countof(file_stack))
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
	int newline;

re_read:
	if(file_stack_idx < 0)
		ICE("file stack idx = 0 on read()");
	f = file_stack[file_stack_idx].file;

	line = fline(f, &newline);

	if(!line){
		if(ferror(f))
			die("read():");

		fclose(f);
		if(file_stack_idx > 0){
			free(dirname_pop());
			preproc_pop();
			goto re_read;
		}else{
			if(!prev_newline){
				CPP_WARN(WNEWLINE, "no newline at end-of-file");
			}
		}

		return NULL;
	}

	prev_newline = newline;
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
	char *anchor = line;

	for(;;){
		char *qmark = strstr(anchor, "??");

		if(!qmark)
			break;

		anchor = qmark + 1;

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
	char *last_backslash;
	int splice = 0;

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


	last_backslash = strrchr(line, '\\');
	if(last_backslash){
		char *i;
		/* is this the end of the line? */
		i = str_spc_skip(last_backslash + 1);
		if(*i == '\0'){
			splice = 1;

			if(i > last_backslash + 1){
				CPP_WARN(WBACKSLASH_SPACE_NEWLINE,
						"backslash and newline separated by space");
			}
		}
	}

	if(splice){
		char *next = splice_lines(peof);

		/* remove in any case */
		*last_backslash = '\0';

		if(next){
			const size_t this_len = last_backslash - line;
			const size_t next_len = strlen(next);

			n_nls++;

			line = urealloc1(line, this_len + next_len + 1);
			strcpy(line + this_len, next);
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

	if(strip_in_block == IN_BLOCK_BEGIN)
		strip_in_block = IN_BLOCK_FULL;
	else if(strip_in_block == IN_BLOCK_END)
		strip_in_block = NOT_IN_BLOCK;

	for(s = line; *s; s++){
		if(strip_in_block == IN_BLOCK_FULL || strip_in_block == IN_BLOCK_BEGIN){
			const int clear_comments = (strip_comments == STRIP_ALL);

			if(*s == '*' && s[1] == '/'){
				strip_in_block = IN_BLOCK_END;
				if(clear_comments)
					*s++ = ' ';
			}
			if(clear_comments)
				*s = ' ';

		}else if(*s == '"' || *s == '\''){
			/* read until the end of the string */
			char *end = str_quotefin2(s + 1, *s);

			if(!end){
				CPP_WARN(WQUOTE, "no terminating quote to string");
				/* skip to eol */
				s = strchr(s, '\0') - 1;
			}else{
				s = end;
			}

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
				strip_in_block = IN_BLOCK_BEGIN;

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
		switch(strip_in_block){
			case NOT_IN_BLOCK:
			case IN_BLOCK_BEGIN:
			case IN_BLOCK_END:
				line = filter_macros(line);
			case IN_BLOCK_FULL: /* no thanks */
				break;
		}

		debug_pop_line();

		if(line){
			if(!no_output)
				puts(line);
			free(line);
		}
	}

	switch(strip_in_block){
		case NOT_IN_BLOCK:
		case IN_BLOCK_END:
			break;
		case IN_BLOCK_BEGIN:
		case IN_BLOCK_FULL:
			CPP_DIE("no terminating block comment");
	}

	parse_end_validate();
}
