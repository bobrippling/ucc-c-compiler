#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/util.h"

#include "tokenise.h"
#include "str.h"

/* start must be free'd if not returned */
token **tokenise(char *line)
{
	token **tokens;
	token *t;
	char *p;

	tokens = NULL;

	for(p = line; *p; p++){
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

char *tokens_join_n(token **tokens, int lim)
{
	int i;
	int len;
	char *val;

#define LIM_TEST(toks, idx, lim) toks[idx] && (lim == -1 || idx < lim)

	len = 1;
	for(i = 0; LIM_TEST(tokens, i, lim); i++)
		len += 1 + strlen(token_str(tokens[i]));
	val = umalloc(len);
	*val = '\0';
	for(i = 0; LIM_TEST(tokens, i, lim); i++){
		if(tokens[i]->had_whitespace)
			strcat(val, " ");
		strcat(val, token_str(tokens[i]));
	}

	return val;
}

char *tokens_join(token **tokens)
{
	return tokens_join_n(tokens, -1);
}
