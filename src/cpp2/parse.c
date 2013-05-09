#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include "parse.h"
#include "macro.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/util.h"
#include "preproc.h"
#include "main.h"
#include "str.h"

#define SINGLE_TOKEN(err) \
	if(dynarray_count(tokens) != 1 || tokens[0]->tok != TOKEN_WORD) \
		CPP_DIE(err)

#define NO_TOKEN(err) \
	if(dynarray_count(tokens)) \
		CPP_DIE(err)

#define NOOP_RET() if(should_noop()) return

#define SHOW_TOKENS(pre, t) \
	do{ \
		int i, len; \
		for(len = 0; t[i]; i++) \
			fprintf(stderr, pre "token %d = %s (whitespace = %c)\n", i, token_str(t[i]), t[i]->had_whitespace["NY"]); \
	}while(0)


char ifdef_stack[32] = { 0 };
int  ifdef_idx = 0;
int noop = 0;


int should_noop(void)
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

token **tokenise(char *line)
{
	token **tokens;
	token *t;
	char *p;

	tokens = NULL;

	for(p = line + 1; *p; p++){
		char c;

		t = umalloc(sizeof *t);
		dynarray_add(&tokens, t);

		while(isspace(*p)){
			t->had_whitespace = 1;
			p++;
		}

		if(!*p){
			t->w = ustrdup("");
			break;
		}

		c = *p;
		if(isalpha(c) || c == '_'){
			char *start;
word:
			start = p;

			t->tok = TOKEN_WORD;

			for(p++; *p; p++)
				if(!(isalnum(*p) || *p == '_'))
					break;

			c = *p;
			*p = '\0';
			t->w = ustrdup(start);
			*p = c;
			p--;
		}else if(c == ','){
			t->tok = TOKEN_COMMA;
		}else if(c == '('){
			t->tok = TOKEN_OPEN_PAREN;
		}else if(c == ')'){
			t->tok = TOKEN_CLOSE_PAREN;
			p++;
			break; /* exit early */
		}else if(!strncmp(p, "...", 3)){
			t->tok = TOKEN_ELIPSIS;
			p += 2;
		}else if(c == '"'){
			char *end  = strchr(p + 1, '"');
			char c;

			/* guaranteed, since strip_comment() checks */
			while(end[-1] == '\\')
				end = strchr(end + 1, '"');

			c = end[1];
			end[1] = '\0';
			t->w = ustrdup(p);
			end[1] = c;
			p = end;

			t->tok = TOKEN_WORD;
		}else{
			goto word;
		}
	}

	if(*p){
		t = umalloc(sizeof *t);
		dynarray_add(&tokens, t);
		if(isspace(*p))
			t->had_whitespace = 1;
		t->tok = TOKEN_OTHER;
		t->w = ustrdup(p);
	}

	/* trim tokens */
	if(tokens){
		int i;
		for(i = 0; tokens[i]; i++)
			if(tokens[i]->w)
				str_trim(tokens[i]->w);
	}

	return tokens;
}

const char *token_str(token *t)
{
	switch(t->tok){
		case TOKEN_OTHER:
		case TOKEN_WORD:
			if(!t->w)
				ICE("no string for token word");
			return t->w;

#define MAP(e, c) case e: return c
		MAP(TOKEN_OPEN_PAREN,  "(");
		MAP(TOKEN_CLOSE_PAREN, ")");
		MAP(TOKEN_COMMA,       ",");
		MAP(TOKEN_ELIPSIS,   "...");
#undef MAP
	}

	ICE("invalid token %d", t->tok);
	return NULL;
}

char *tokens_join(token **tokens)
{
	int i;
	int len;
	char *val;

	len = 1;
	for(i = 0; tokens[i]; i++)
		len += 1 + strlen(token_str(tokens[i]));
	val = umalloc(len);
	*val = '\0';
	for(i = 0; tokens[i]; i++){
		if(tokens[i]->had_whitespace)
			strcat(val, " ");
		strcat(val, token_str(tokens[i]));
	}

	return val;
}

void handle_define(token **tokens)
{
	char *name;

	if(tokens[0]->tok != TOKEN_WORD)
		CPP_DIE("word expected");

	NOOP_RET();

	name = tokens[0]->w;

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

void handle_undef(token **tokens)
{
	SINGLE_TOKEN("invalid undef macro");

	NOOP_RET();

	macro_remove(tokens[0]->w);
}

void handle_error_warning(token **tokens, const char *pre)
{
	char *s;

	s = tokens_join(tokens);

	preproc_backtrace();

	CPP_WARN("#%s: %s", pre, s);

	free(s);
}

void handle_warning(token **tokens)
{
	NOOP_RET();
	handle_error_warning(tokens, "warning");
}

void handle_error(token **tokens)
{
	NOOP_RET();
	handle_error_warning(tokens, "error");
	exit(1);
}

void handle_include(token **tokens)
{
	FILE *f;
	char *fname;
	int len, free_fname, lib;

	extern char **dirnames;
	char *dname;
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

	i = dynarray_count((void **)dirnames);
	dname = dirnames[i - 1];

	if(*fname == '/'){
		/* absolute path */
		path = ustrdup(fname);
		goto abs_path;
	}

	i = dynarray_count(dirnames);
	dname = dirnames[i - 1];

	if(lib){
lib:
		f = NULL;

		for(i = 0; lib_dirs && lib_dirs[i]; i++){
			path = ustrprintf("%s/%s/%s",
					*lib_dirs[i] == '/' ? "" : dname,
					lib_dirs[i], fname);
			f = fopen(path, "r");
			if(f)
				break;
		}

		if(!f)
			CPP_DIE("can't find include file %c%s%c",
					"\"<"[lib], fname, "\">"[lib]);

		if(option_debug)
			fprintf(stderr, ">>> include lib: %s\n", path);
	}else{
		path = ustrprintf("%s/%s", dname, fname);
abs_path:
		f = fopen(path, "r");
		if(!f){
			/* attempt lib */
			goto lib;
		}
		if(option_debug)
			fprintf(stderr, ">>> include \"%s/%s\"\n", dname, fname);
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

void handle_somedef(token **tokens, int rev)
{
	SINGLE_TOKEN("invalid ifdef macro");

	ifdef_push(rev ^ !macro_find(tokens[0]->w));
}

void handle_ifdef(token **tokens)
{
	handle_somedef(tokens, 0);
}

void handle_ifndef(token **tokens)
{
	handle_somedef(tokens, 1);
}

void handle_if(token **tokens)
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

void handle_else(token **tokens)
{
	NO_TOKEN("invalid else macro");

	if(ifdef_idx == 0)
		CPP_DIE("else unexpected");

	noop = !noop;
}

void handle_endif(token **tokens)
{
	NO_TOKEN("invalid endif macro");

	if(ifdef_idx == 0)
		CPP_DIE("endif unexpected");

	ifdef_pop();
}

void handle_pragma(token **tokens)
{
	(void)tokens;
}

void handle_macro(char *line)
{
	token **tokens;
	int i;

	tokens = tokenise(line);

	if(!tokens)
		return;

	if(tokens[0]->tok != TOKEN_WORD){
		if(should_noop())
			return;

		CPP_DIE("invalid preproc token");
	}

	DEBUG(DEBUG_NORM, "macro %s\n", tokens[0]->w);

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

	if(should_noop())
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

void macro_finish()
{
	if(ifdef_idx)
		CPP_DIE("endif expected");
}
