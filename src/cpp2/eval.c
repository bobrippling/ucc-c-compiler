#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "eval.h"

#include "../util/dynarray.h"
#include "../util/util.h"
#include "../util/alloc.h"

#include "tokenise.h"
#include "macro.h"
#include "str.h"
#include "main.h"

static char **split_func_args(char *args_str)
{
	token **tokens = tokenise(args_str, 0 /* don't stop at ')' */);
	token **ti, **anchor = tokens;
	char **args = NULL;

	for(ti = tokens; ti && *ti; ti++){
		token *t = *ti;

		if(t->tok == TOKEN_COMMA){
			char *arg = tokens_join_n(anchor, ti - anchor);
			dynarray_add(&args, arg);
			anchor = ti + 1;
		}
	}

	if(anchor != ti)
		dynarray_add(&args, tokens_join_n(anchor, ti - anchor));

	return args;
}

#if 0
		if(!strcmp(arg_target, VA_ARGS_STR)){
			char *quoted, *free_me;
			const int offset = s - replace;

			found = 1;

			if(!args)
				CPP_DIE("#" VA_ARGS_STR " used on non-variadic macro");

			quoted = str_quote(free_me = str_join(args, ", "));
			free(free_me);

			replace = str_replace(replace,
					hash,
					s + strlen(VA_ARGS_STR),
					quoted);

			free(quoted);
			last = replace + offset + strlen(quoted);
		}
#endif

static char *eval_word(macro *m, char *word, char **args)
{
	if(m->args){
		char *w;
		int i;

		for(i = 0; (w = m->args[i]); i++)
			if(!strcmp(w, word))
				return args[i];
	}

	/* word not found, we use the given identifier */
	return word;
}

static char *eval_hash(
		macro *m, token *this, char **args, int need_arg, const char *emsg)
{
	char *evalled;

	if(!this)
		CPP_DIE("nothing to %s after '#' character", emsg);
	if(this->tok != TOKEN_WORD)
		CPP_DIE("can't %s a none-word (%s)", emsg, token_str(this));

	evalled = eval_word(m, this->w, args);
	if(need_arg && evalled == this->w){
		/* not found */
		CPP_DIE("can't %s non-argument \"%s\"", emsg, evalled);
	}

	return evalled;
}


static char *eval_func_macro(macro *m, char *args_str)
{
	char **args = split_func_args(args_str);

	int got = dynarray_count(args)
		, exp = dynarray_count(m->args);

	if(m->type == VARIADIC ? got <= exp : got != exp){
		CPP_DIE("wrong number of args to function macro \"%s\", got %d, expected %d",
				m->nam, got, exp);
	}

	if(args){
		int i;
		for(i = 0; args[i]; i++)
			str_trim(args[i]);
	}

	{
		token **toks = tokenise(m->val, 0);
		token **ti;
		char *replace = ustrdup("");

#define APPEND(ws, fmt, ...)                        \
				do{                                         \
					char *new = ustrprintf("%s%s" fmt,        \
							replace,                              \
							*replace && ws ? " " : "",            \
							__VA_ARGS__);                         \
					free(replace), replace = new;             \
					break;                                    \
				}while(0)

		for(ti = toks; ti && *ti; ti++){
			token *this = *ti;
			switch(this->tok){
				case TOKEN_HASH_QUOTE:
					/* replace #arg with the quote of arg */
					APPEND(this->had_whitespace, "\"%s\"",
							eval_hash(m, *++ti, args, 1, "quote"));
					break;

				case TOKEN_HASH_JOIN:
					/* replace a ## b with the join of both */
					CPP_DIE("## with no prior argument");

				case TOKEN_WORD:
				{
					char *word;

					if(ti[1] && ti[1]->tok == TOKEN_HASH_JOIN){
						word = eval_hash(m, ti[0], args, 0, "join");

						ti++;

						while(*ti && ti[0]->tok == TOKEN_HASH_JOIN){
							char *old = word;

							word = ustrprintf("%s%s", word,
									eval_hash(m, ti[1], args, 0, "join"));

							free(old);

							ti += 2;
						}
						ti--;
					}else{
						word = eval_word(m, this->w, args);
					}

					APPEND(this->had_whitespace, "%s", word);
					free(word);
					break;
				}

				case TOKEN_OPEN_PAREN:
				case TOKEN_CLOSE_PAREN:
				case TOKEN_COMMA:
				case TOKEN_ELIPSIS:
				case TOKEN_STRING:
				case TOKEN_OTHER:
					APPEND(this->had_whitespace, "%s", token_str(this));
			}
		}


		/* TODO: free args */
		return replace;
	}
#undef APPEND
}

static char *eval_macro_r(macro *m, char *start, char *at)
{
	if(m->type == MACRO){
		static int counter = 0; /* __COUNTER__ */
		int free_val = 0;
		char *val;
		char *ret;

		if(m->val){
			val = m->val;
		}else{
			free_val = 1;

			if(!strcmp(m->nam, "__FILE__")){
				val = ustrprintf("\"%s\"", current_fname);
			}else if(!strcmp(m->nam, "__LINE__")){
				val = ustrprintf("%d", current_line);
			}else if(!strcmp(m->nam, "__COUNTER__")){
				val = ustrprintf("%d", counter++);
			}else if(!strcmp(m->nam, "__DATE__")){
				free_val = 0;
				val = cpp_date;
			}else if(!strcmp(m->nam, "__TIME__")){
				free_val = 0;
				val = cpp_time;
			}else{
				ICE("invalid macro");
			}
		}

		ret = word_replace(start, at, strlen(m->nam), val);

		if(free_val)
			free(val);

		return ret;

	}else{
		char *open_b, *close_b;

		for(open_b = at + strlen(m->nam); isspace(*open_b); open_b++);

		if(*open_b != '(')
			return start; /* not an invocation */

		close_b = strchr_nest(open_b, ')');
		if(!close_b)
			CPP_DIE("unterminated function-macro '%s'", m->nam);

		{
			char *all_args = ustrdup2(open_b + 1, close_b);
			char *eval_d = eval_func_macro(m, all_args);
			char *ret = str_replace(start, at, close_b + 1, eval_d);

			free(all_args);
			free(eval_d);

			return ret;
		}
	}
}

static char *eval_macro(macro *m, char *start, char *at)
{
	char *r;
	if(m->blue)
		return start;

	m->blue = 1;
	r = eval_macro_r(m, start, at);
	m->blue = 0;
	return r;
}

char *eval_expand_macros(char *line)
{
	size_t i;

	for(i = 0; line[i]; i++){
		char *end, save;
		macro *m;

		if(!iswordpart(line[i]))
			continue;

		end = word_end(line + i);
		save = *end, *end = '\0';
		m = macro_find(line + i);
		*end = save;

		if(m)
			line = eval_macro(m, line, line + i);
	}

	return line;
}
