#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"

#include "parse.h"
#include "tokenise.h"
#include "main.h"
#include "macro.h"
#include "preproc.h"

#define SINGLE_TOKEN(err) \
	if(dynarray_count(tokens) != 1 || tokens[0]->tok != TOKEN_WORD) \
		CPP_DIE(err)

#define NO_TOKEN(err) \
	if(dynarray_count(tokens)) \
		CPP_DIE(err)

#define NOOP_RET() if(parse_should_noop()) return


static char ifdef_stack[32] = { 0 };
static int  ifdef_idx = 0;
static int noop = 0;


int parse_should_noop(void)
{
	int i;

	/*
	 * all must be zero to noop
	 * i.e. every noop for each ifdef of the current position
	 * should be 0, meaning no noops
	 */
	if(noop)
		return 1;

	/* TODO: optimise with memcmp? */
	for(i = 0; i < ifdef_idx; i++)
		if(ifdef_stack[i])
			return 1;

	return 0;
}

static void handle_define(token **tokens)
{
	char *name;

	if(tokens[0]->tok != TOKEN_WORD)
		CPP_DIE("word expected");

	NOOP_RET();

	name = tokens[0]->w;

	if(tokens[1]) /* prevent initial whitespace */
		tokens[1]->had_whitespace = 0;

	if(tokens[1] && tokens[1]->tok == TOKEN_OPEN_PAREN && !tokens[1]->had_whitespace){
		/* function macro */
		int i, variadic;
		char **args;
		char *val;

		variadic = 0;
		args = NULL;

		for(i = 2; tokens[i]; i++){
			switch(tokens[i]->tok){
				case TOKEN_CLOSE_PAREN:
					i++;
					goto for_fin;

				case TOKEN_ELIPSIS:
					variadic = 1;
					i++;
					if(tokens[i]->tok != TOKEN_CLOSE_PAREN)
						CPP_DIE("expected: close paren");
					i++;
					goto for_fin;

				case TOKEN_WORD:
					dynarray_add(&args, ustrdup(token_str(tokens[i])));

					i++;
					switch(tokens[i]->tok){
						case TOKEN_COMMA:
							continue;
						case TOKEN_CLOSE_PAREN:
							i++;
							goto for_fin;
						default:
							CPP_DIE("expected: comma or close paren");
					}

				default:
					CPP_DIE("unexpected token %s", token_str(tokens[i]));
			}
		}
for_fin:
		if(!tokens[i])
			val = ustrdup("");
		else
			val = tokens_join(tokens + i);

		macro_add_func(name, val, args, variadic);

		free(val);

	}else{
		char *val;
		val = tokens_join(tokens + 1);

		macro_add(name, val);

		free(val);
	}
}

static void handle_undef(token **tokens)
{
	SINGLE_TOKEN("invalid undef macro");

	NOOP_RET();

	macro_remove(tokens[0]->w);
}

static void handle_error_warning(token **tokens, const char *pre)
{
	char *s;

	s = tokens_join(tokens);

	preproc_backtrace();

	CPP_WARN("#%s: %s", pre, s);

	free(s);
}

static void handle_warning(token **tokens)
{
	NOOP_RET();
	handle_error_warning(tokens, "warning");
}

static void handle_error(token **tokens)
{
	NOOP_RET();
	handle_error_warning(tokens, "error");
	exit(1);
}

static void handle_include(token **tokens)
{
	FILE *f;
	char *fname;
	int len, free_fname, lib;

	const char *current_include_dname;
	char *path = NULL;
	int i;

	free_fname = 0;

	if(tokens[1]){
		int i;

		len = 0;

		for(i = 0; tokens[i]; i++)
			len += strlen(token_str(tokens[i]));

		fname = umalloc(len + 1);
		*fname = '\0';

		for(i = 0; tokens[i]; i++)
			strcat(fname, token_str(tokens[i]));

		free_fname = 1;
	}else{
		fname = tokens[0]->w;
		len = strlen(fname);
	}

retry:
	if(*fname == '<'){
		if(fname[len-1] != '>')
			CPP_DIE("invalid include end '%c'", fname[len-1]);
	}else if(*fname != '"'){
		/*
		 * #define a "hi"
		 * #include a
		 * pretty annoying..
		 */
		macro *m;

		m = macro_find(fname);
		if(!m)
			CPP_DIE("invalid include start \"%s\" (not <xyz>, \"xyz\" or a macro)", fname);

		for(fname = m->val; isspace(*fname); fname++);
		len = strlen(fname);
		goto retry;
	}
	/* if it's '"' then we've got a finishing '"' */

	NOOP_RET(); /* should be higher up? */

	lib = *fname == '<';
	fname[len-1] = '\0';
	fname++;

	i = dynarray_count(cd_stack);
	current_include_dname = cd_stack[i - 1];

	if(*fname == '/'){
		/* absolute path */
		path = ustrdup(fname);
		goto abs_path;
	}

	if(lib){
lib:
		f = include_fopen(current_include_dname, fname);

		if(!f)
			CPP_DIE("can't find include file %c%s%c",
					"\"<"[lib], fname, "\">"[lib]);
	}else{
		path = ustrprintf("%s/%s", current_include_dname, fname);
abs_path:
		f = fopen(path, "r");
		if(!f){
			/* attempt lib */
			goto lib;
		}
	}

	preproc_push(f, path);
	dirname_push(udirname(path));
	free(path);

	if(free_fname)
		free(fname - 1);
}

static void ifdef_push(int new)
{
	ifdef_stack[ifdef_idx++] = noop;

	if(ifdef_idx == sizeof ifdef_stack)
		die("ifdef stack exceeded");

	noop = new;
}

static void ifdef_pop(void)
{
	if(ifdef_idx == 0)
		ICE("ifdef_idx == 0 on ifdef_pop()");

	noop = ifdef_stack[--ifdef_idx];
}

static void handle_somedef(token **tokens, int rev)
{
	SINGLE_TOKEN("invalid ifdef macro");

	ifdef_push(rev ^ !macro_find(tokens[0]->w));
}

static void handle_ifdef(token **tokens)
{
	handle_somedef(tokens, 0);
}

static void handle_ifndef(token **tokens)
{
	handle_somedef(tokens, 1);
}

static void handle_if(token **tokens)
{
	const char *str = tokens[0]->w;
	int test;

	SINGLE_TOKEN("limited if macro");

	/* currently only check for #if 0 */
	test = !strcmp(str, "0");

	if(!test)
		fprintf(stderr, "\"#if %s\" assuming pass\n", str);

	ifdef_push(test);
}

static void handle_else(token **tokens)
{
	NO_TOKEN("invalid else macro");

	if(ifdef_idx == 0)
		CPP_DIE("else unexpected");

	noop = !noop;
}

static void handle_endif(token **tokens)
{
	NO_TOKEN("invalid endif macro");

	if(ifdef_idx == 0)
		CPP_DIE("endif unexpected");

	ifdef_pop();
}

static void handle_pragma(token **tokens)
{
	(void)tokens;
}

void parse_directive(char *line)
{
	token **tokens;
	int i;

	tokens = tokenise(line, 1 /* only until ')' */);

	if(!tokens)
		return;

	if(tokens[0]->tok != TOKEN_WORD){
		if(parse_should_noop())
			return;

		CPP_DIE("invalid preproc token");
	}

	/* check for '# [0-9]+ "..."' */
	if(sscanf(tokens[0]->w, "%d \"", &i) == 1){
		/* output, and ignore */
		puts(line);
		return;
	}

	putchar('\n'); /* keep line-no.s in sync */

#define HANDLE(s)                \
	if(!strcmp(tokens[0]->w, #s)){ \
		handle_ ## s(tokens + 1);    \
		goto fin;                    \
	}

	HANDLE(ifdef)
	HANDLE(ifndef)
	HANDLE(if)
	HANDLE(else)
	HANDLE(endif)

	if(parse_should_noop())
		return; /* checked for flow control, nothing else so noop */

	HANDLE(include)

	HANDLE(define)
	HANDLE(undef)

	HANDLE(warning)
	HANDLE(error)

	HANDLE(pragma)

	CPP_DIE("unrecognised preproc command \"%s\"", tokens[0]->w);
fin:
	for(i = 0; tokens[i]; i++){
		free(tokens[i]->w);
		free(tokens[i]);
	}
	free(tokens);
}

void parse_end_validate()
{
	if(ifdef_idx)
		CPP_DIE("endif expected");
}
