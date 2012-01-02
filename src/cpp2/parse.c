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

#define SINGLE_TOKEN(err) \
	if(dynarray_count((void **)tokens) != 1 || tokens[0]->tok != TOKEN_WORD) \
		die(err, dynarray_count((void **)tokens))

#define NO_TOKEN(err) \
	if(dynarray_count((void **)tokens)) \
		die(err)

#define NOOP_RET() if(should_noop()) return


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
		dynarray_add((void ***)&tokens, t);

		while(isspace(*p)){
			t->had_whitespace = 1;
			p++;
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
			break;
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
		dynarray_add((void ***)&tokens, t);
		t->tok = TOKEN_OTHER;
		t->w = ustrdup(p);
	}

	return tokens;
}

const char *token_str(token *t)
{
	switch(t->tok){
		case TOKEN_OTHER:
		case TOKEN_WORD:
			return t->w;

#define MAP(e, c) case e: return c
		MAP(TOKEN_OPEN_PAREN,  "(");
		MAP(TOKEN_CLOSE_PAREN, ")");
		MAP(TOKEN_COMMA,       ",");
#undef MAP
	}

	die("invalid token %d", t->tok);
	return NULL;
}

void handle_define(token **tokens)
{
	char *name;

	if(tokens[0]->tok != TOKEN_WORD)
		die("word expected");

	NOOP_RET();

	name = tokens[0]->w;

	if(tokens[1] && tokens[1]->tok == TOKEN_OPEN_PAREN && !tokens[1]->had_whitespace){
		/* function macro */
		TODO();

	}else{
		char *val;
		int i;
		int len;

		len = 1;
		for(i = 1; tokens[i]; i++)
			len += strlen(token_str(tokens[i]));
		val = umalloc(len);
		*val = '\0';
		for(i = 1; tokens[i]; i++)
			strcat(val, token_str(tokens[i]));

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

void handle_include(token **tokens)
{
	FILE *f;
	char *fname;
	int len;

	SINGLE_TOKEN("invalid include macro");

	fname = tokens[0]->w;
	len = strlen(fname);

	if(*fname == '<' && fname[len-1] != '>')
		die("invalid include end");
	else if(*fname != '"')
		die("invalid include start");

	/* if it's '"' then we've got a finishing '"' */

	if(*fname == '<')
		TODO();

	NOOP_RET();

	fname[len-1] = '\0';
	fname++;

	f = fopen(fname, "r");
	if(!f)
		die("open %s: %s", fname, strerror(errno));
	preproc_push(f);
}

void ifdef_push(int val)
{
	ifdef_stack[ifdef_idx++] = val;

	if(ifdef_idx == sizeof ifdef_stack)
		die("ifdef stack exceeded");
}

void ifdef_pop(void)
{
	if(ifdef_idx == 0)
		ICE("ifdef_idx == 0 on ifdef_pop()");

	noop = ifdef_stack[--ifdef_idx];
}

void handle_somedef(token **tokens, int rev)
{
	SINGLE_TOKEN("invalid ifdef macro");

	ifdef_push(noop);

	noop = rev ^ !macro_find(tokens[0]->w);
}

void handle_ifdef(token **tokens)
{
	handle_somedef(tokens, 0);
}

void handle_ifndef(token **tokens)
{
	handle_somedef(tokens, 1);
}

void handle_else(token **tokens)
{
	NO_TOKEN("invalid else macro");

	if(ifdef_idx == 0)
		die("else unexpected");

	noop = !noop;
}

void handle_endif(token **tokens)
{
	NO_TOKEN("invalid endif macro");

	if(ifdef_idx == 0)
		die("endif unexpected");

	ifdef_pop();
}

void handle_macro(char *line)
{
	token **tokens;
	int i;

	tokens = tokenise(line);

	if(!tokens)
		return;

	if(tokens[0]->tok != TOKEN_WORD)
		die("invalid preproc token");

#define MAP(s, f)                \
	if(!strcmp(tokens[0]->w, s)){  \
		f(tokens + 1);               \
		goto fin;                    \
	}

	DEBUG(DEBUG_NORM, "macro %s\n", tokens[0]->w);

	MAP("define",  handle_define)
	MAP("undef",   handle_undef)

	MAP("include", handle_include)

	MAP("ifdef",   handle_ifdef)
	MAP("ifndef",  handle_ifndef)
	MAP("else",    handle_else)
	MAP("endif",   handle_endif)

	die("unrecognised preproc command \"%s\"", tokens[0]->w);
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
		die("endif expected");
}
