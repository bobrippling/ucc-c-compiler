#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "macro.h"

#define DIE(x) fprintf(stderr, "%s\n", x); exit(1);

struct token
{
	enum
	{
		TOKEN_WORD,
		TOKEN_OPEN_PAREN,
		TOKEN_CLOSE_PAREN,
		TOKEN_COMMA,
		TOKEN_OTHER
	} tok;
	char *w;
	int had_whitespace;
};


void macro_add_dir(const char *d)
{
	(void)d;
	DIE("TODO");
}

void macro_add(const char *nam, const char *val)
{
	(void)nam;
	(void)val;
	DIE("TODO");
}

struct token **tokenise(char *line)
{
	struct token **tokens;
	struct token *t;
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

	t = umalloc(sizeof *t);
	dynarray_add((void ***)&tokens, t);
	t->tok = TOKEN_OTHER;
	t->w = ustrdup(p);

	return tokens;
}

void filter_macro(char **pline)
{
	(void)pline;
}

void handle_define(struct token **tokens)
{
	char *name;

	if(tokens[0]->tok != TOKEN_WORD)
		die("word expected");

	name = tokens[0]->w;

	if(tokens[1]->tok == TOKEN_OPEN_PAREN){
		/* function macro */

	}else{
		char *val;
		int i;
		int len;

		len = 1;
		for(i = 1; tokens[i]; i++)
			len += strlen(tokens[i].w);
	}
}

void handle_include(struct token **tokens)
{
}

void handle_ifdef(struct token **tokens)
{
}

void handle_else(struct token **tokens)
{
}

void handle_endif(struct token **tokens)
{
}

void handle_macro(char **pline)
{
	char *line = *pline;
	struct token **tokens;
	int i;

	tokens = tokenise(line);
	i = 0;

	if(tokens[0]->tok != TOKEN_WORD)
		die("invalid preproc token");

#define MAP(s, f)               \
	if(!strcmp(tokens[0]->w, s)){  \
		f(tokens + 1);              \
		return;                     \
	}

	MAP("define",  handle_define);
	MAP("include", handle_include);
	MAP("ifdef",   handle_ifdef);
	MAP("else",    handle_else);
	MAP("endif",   handle_endif);

	die("unrecognised preproc command");
}
