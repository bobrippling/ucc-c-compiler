#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/util.h"
#include "../util/str.h"

#include "tokenise.h"
#include "str.h"

/* start must be free'd if not returned */
token **tokenise(char *line)
{
	token **tokens = NULL;
	char *p;

	for(p = line; *p; p++){
		token *t = umalloc(sizeof *t);
		char c;

		dynarray_add(&tokens, t);

		if(isspace(*p))
			t->had_whitespace = 1;
		p = str_spc_skip(p);

		if(!*p){
			t->w = ustrdup("");
			break;
		}

		c = *p;
		if(isalpha(c) || c == '_'){
			const char *start = p;

			t->tok = TOKEN_WORD;

			for(p++; *p; p++)
				if(!(isalnum(*p) || *p == '_'))
					break;

			t->w = ustrdup2(start, p);
			p--;

		}else switch(c){
			case ',':
				t->tok = TOKEN_COMMA;
				break;
			case '(':
				t->tok = TOKEN_OPEN_PAREN;
				break;
			case ')':
				t->tok = TOKEN_CLOSE_PAREN;
				break;
			case '"':
			{
				char *end = str_quotefin(p + 1);

				/* guaranteed, since strip_comment() checks */
				UCC_ASSERT(end, "strip_comment() broken for >>>%s<<<", p);

				t->w = ustrdup2(p, end + 1);
				p = end;

				t->tok = TOKEN_STRING;
				break;
			}
			case '#':
				if(p[1] == '#')
					t->tok = TOKEN_HASH_JOIN, p++;
				else
					t->tok = TOKEN_HASH_QUOTE;
				break;
			default:
				if(!strncmp(p, "...", 3)){
					t->tok = TOKEN_ELIPSIS;
					p += 2;
				}else{
					t->tok = TOKEN_OTHER;
					t->w = ustrdup2(p, p + 1);
				}
		}
	}

	if(*p){
		token *t = umalloc(sizeof *t);
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
		case TOKEN_STRING:
			UCC_ASSERT(t->w, "no word");
			return t->w;

#define MAP(e, c) case e: return c
		MAP(TOKEN_OPEN_PAREN,  "(");
		MAP(TOKEN_CLOSE_PAREN, ")");
		MAP(TOKEN_COMMA,       ",");
		MAP(TOKEN_ELIPSIS,   "...");
		MAP(TOKEN_HASH_QUOTE,  "#");
		MAP(TOKEN_HASH_JOIN,  "##");
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

void tokens_free(token **tokens)
{
	int i;
	if(!tokens)
		return;

	for(i = 0; tokens[i]; i++){
		free(tokens[i]->w);
		free(tokens[i]);
	}
	free(tokens);
}

static int token_is_space(token *t)
{
	switch(t->tok){
		case TOKEN_WORD:
			if(*str_spc_skip(t->w))
				return 0;
			break;
		case TOKEN_OPEN_PAREN:
		case TOKEN_CLOSE_PAREN:
		case TOKEN_COMMA:
		case TOKEN_ELIPSIS:
		case TOKEN_STRING:
		case TOKEN_HASH_QUOTE:
		case TOKEN_HASH_JOIN:
		case TOKEN_OTHER:
			return 0;
	}
	return 1;
}

token **tokens_skip_whitespace(token **tokens)
{
	for(; tokens && *tokens; tokens++)
		if(!token_is_space(*tokens))
			return tokens;
	return tokens;
}

int tokens_just_whitespace(token **tokens)
{
	return tokens_skip_whitespace(tokens) ? 0 : 1;
}

int tokens_count_skip_spc(token **tokens)
{
	unsigned n = 0;

	for(; tokens && *tokens; tokens++)
		if(!token_is_space(*tokens))
			n++;

	return n;
}
