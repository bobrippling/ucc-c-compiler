#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "../util/dynarray.h"
#include "str.h"
#include "macro.h"

#define SINGLE_TOKEN(err) \
	if(dynarray_count((void **)tokens) != 1 || tokens[0]->tok != TOKEN_WORD) \
		die(err, dynarray_count((void **)tokens))

#define NO_TOKEN(err) \
	if(dynarray_count((void **)tokens)) \
		die(err)

enum
{
	DEBUG_NORM = 0,
	DEBUG_VERB = 1,
};
#define DEBUG(level, ...) if(level < debug) fprintf(stderr, ">>> " __VA_ARGS__)

#define TODO() fprintf(stderr, "%s: TODO\n", __func__); exit(1)

typedef struct
{
	enum tok
	{
		TOKEN_WORD,
		TOKEN_OPEN_PAREN,
		TOKEN_CLOSE_PAREN,
		TOKEN_COMMA,
		TOKEN_OTHER
	} tok;
	char *w;
	int had_whitespace;
} token;


typedef struct
{
	char *nam, *val;
	char **args;
} macro;


/* vars */
extern int debug;

macro **macros = NULL;
char ifdef_stack[32] = { 0 };
int  ifdef_idx = 0;
int noop = 0;


void macro_add_dir(const char *d)
{
	(void)d;
	TODO();
}

macro *macro_find(const char *sp)
{
	macro **i;
	for(i = macros; i && *i; i++)
		if(!strcmp((*i)->nam, sp))
			return *i;
	return 0;
}

void macro_add(const char *nam, const char *val)
{
	macro *m;

	m = macro_find(nam);

	if(m){
		fprintf(stderr, "cpp: warning: redefining \"%s\"\n", nam);
		free(m->nam);
		free(m->val);
	}else{
		m = umalloc(sizeof *m);
		dynarray_add((void ***)&macros, m);
	}

	m->nam = ustrdup(nam);
	m->val = ustrdup(val);

	DEBUG(DEBUG_NORM, "macro_add(\"%s\", \"%s\")\n", nam, val);
}

void macro_remove(const char *nam)
{
	macro *m = macro_find(nam);

	if(m){
		free(m->nam);
		free(m->val);
		dynarray_rm((void ***)macros, m);
		free(m);
	}
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
			char *start = p;

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
		}else{
			break;
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

void filter_macro(char **pline)
{
	char *pos;
	macro **iter;

	if(noop)
		**pline = '\0';

	if(!**pline)
		return; /* optimise for empty lines also */

	for(iter = macros; iter && *iter; iter++){
		macro *m = *iter;

		pos = *pline;
		DEBUG(DEBUG_VERB, "word_find(\"%s\", \"%s\")\n", pos, m->nam);

		while((pos = word_find(pos, m->nam))){
			int posidx = pos - *pline;

			DEBUG(DEBUG_VERB, "word_replace(line=\"%s\", pos=\"%s\", nam=\"%s\", val=\"%s\")\n",
					*pline, pos, m->nam, m->val);

			*pline = word_replace(*pline, pos, m->nam, m->val);
			pos = *pline + posidx + strlen(m->val);

			iter = macros;
		}
	}
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

	return NULL;
}

void handle_define(token **tokens)
{
	char *name;

	if(tokens[0]->tok != TOKEN_WORD)
		die("word expected");

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

	macro_remove(tokens[0]->w);
}

void handle_include(token **tokens)
{
	SINGLE_TOKEN("invalid include macro");
	TODO();
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
		die("internal preprocessor error: ifdef_idx == 0");

	noop = ifdef_stack[--ifdef_idx];
}

void handle_ifdef(token **tokens)
{
	SINGLE_TOKEN("invalid ifdef macro");

	ifdef_push(noop);

	noop = !macro_find(tokens[0]->w);
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

void handle_macro(char **pline)
{
	char *line = *pline;
	token **tokens;

	tokens = tokenise(line);

	if(tokens[0]->tok != TOKEN_WORD)
		die("invalid preproc token");

#define MAP(s, f)                \
	if(!strcmp(tokens[0]->w, s)){  \
		f(tokens + 1);               \
		return;                      \
	}

	DEBUG(DEBUG_NORM, "macro %s\n", tokens[0]->w);

	MAP("define",  handle_define)
	MAP("undef",   handle_undef)

	MAP("include", handle_include)

	MAP("ifdef",   handle_ifdef)
	MAP("else",    handle_else)
	MAP("endif",   handle_endif)

	die("unrecognised preproc command \"%s\"", tokens[0]->w);
}
