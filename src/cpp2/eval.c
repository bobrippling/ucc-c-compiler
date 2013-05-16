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
#include "snapshot.h"

#define VA_ARGS_STR "__VA_ARGS__"

static char **split_func_args(char *args_str)
{
	token **tokens = tokenise(args_str);
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

	tokens_free(tokens);

	return args;
}

static char *find_arg(macro *m, char *word, char **args)
{
	if(!strcmp(word, VA_ARGS_STR)){
		if(args){
			size_t i = dynarray_count(m->args);
			/* if count(args) < i then args[i] is NULL,
			 * which str_join handles
			 */
			/* XXX: memleak, everything other return path returns a +0 pointer */
			return str_join(args + i, ", ");
		}else{
			return "";
		}
	}

	if(m->args){
		char *w;
		size_t i;

		for(i = 0; (w = m->args[i]); i++)
			if(!strcmp(w, word))
				return args[i]; /* don't expand */
	}

	/* word not found, we use the given identifier */
	return word;
}

static char *noeval_hash(
		macro *m, token *this, char **args, int need_arg, const char *emsg)
{
	char *word;

	if(!this)
		CPP_DIE("nothing to %s after '#' character", emsg);
	if(this->tok != TOKEN_WORD)
		CPP_DIE("can't %s a none-word (%s)", emsg, token_str(this));

	word = find_arg(m, this->w, args); /* don't expand */
	if(need_arg && word == this->w){
		/* not found */
		CPP_DIE("can't %s non-argument \"%s\"", emsg, word);
	}

	return word;
}

static char *eval_func_macro(macro *m, char *args_str)
{
	/*
	 * 6.10.3.1/1:
	 * ... after the arguments for the invocation of a function-like
	 * macro have been identified, argument substitution takes
	 * place. A parameter in the replacement list, unless preceded
	 * by a # or ## preprocessing token or followed by a ##
	 * preprocessing token (see below), is replaced by the
	 * corresponding argument after all macros contained therein
	 * have been expanded...
	 */
	char **args = split_func_args(args_str);

	int got = dynarray_count(args)
		, exp = dynarray_count(m->args);

	if(m->type == VARIADIC ? got < exp : got != exp){
		CPP_DIE("wrong number of args to function macro \"%s\", "
				"got %d, expected %s%d",
				m->nam, got, m->type == VARIADIC ? "at least " : "", exp);
	}

	if(args){
		int i;
		for(i = 0; args[i]; i++)
			str_trim(args[i]);
	}

	{
		token **toks = tokenise(m->val);
		token **ti;
		char *replace = ustrdup("");

#define APPEND(ws, fmt, ...)                        \
				do{                                         \
					char *new = ustrprintf("%s%s" fmt,        \
							replace,                              \
							*replace && ws ? " " : "",            \
							__VA_ARGS__);                         \
					free(replace), replace = new;             \
				}while(0)

		for(ti = toks; ti && *ti; ti++){
			token *this = *ti;
			switch(this->tok){
				case TOKEN_HASH_QUOTE:
					/* replace #arg with the quote of arg */
					/* # - don't eval */
					APPEND(this->had_whitespace, "\"%s\"",
							noeval_hash(m, *++ti, args, 1, "quote"));
					break;

				case TOKEN_HASH_JOIN:
					/* replace a ## b with the join of both */
					CPP_DIE("## with no prior argument");

				case TOKEN_WORD:
				{
					int free_word = 0;
					char *word;

					if(ti[1] && ti[1]->tok == TOKEN_HASH_JOIN){
						/* word with ## - don't eval */
						word = noeval_hash(m, ti[0], args, 0, "join");

						ti++;

						while(*ti && ti[0]->tok == TOKEN_HASH_JOIN){
							char *old = word;

							word = ustrprintf("%s%s", word,
									noeval_hash(m, ti[1], args, 0, "join"));

							if(free_word)
								free(old);
							free_word = 1;

							ti += 2;
						}
						ti--;
					}else{
						/* word without # nor ## - we may eval */
						word = find_arg(m, this->w, args);
						if(word != this->w){
							/* is an argument, eval.
							 * "... is replaced by the corresponding argument after all
							 * macros contained therein have been expanded..."
							 */
							word = eval_expand_macros(word);
						}
					}

					APPEND(this->had_whitespace, "%s", word);
					if(free_word)
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


		dynarray_free(char **, &args, free);
		tokens_free(toks);
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

		if(*open_b != '('){
			/* not an invocation - return and also knock down the use-count */
			m->use_cnt--;
			return start;
		}

		close_b = strchr_nest(open_b, ')');
		if(!close_b)
			CPP_DIE("unterminated function-macro '%s'", m->nam);

		{
			char *all_args = ustrdup2(open_b + 1, close_b);
			char *eval_d = eval_func_macro(m, all_args);

			free(all_args);

			start = str_replace(start, at, close_b + 1, eval_d);

			free(eval_d);

			return start;
		}
	}
}

static char *eval_macro_double_eval(macro *m, char *start, char *at)
{
	/* FIXME: need to snapshot *m too? i.e. snapshot one level higher */
	snapshot *snapshot = snapshot_take();

	start = eval_macro_r(m, start, at);

	/* mark any macros that changed as blue, to prevent re-evaluation */
	snapshot_take_post(snapshot);
	snapshot_blue_used(snapshot);
	{
		/* double eval */
		start = eval_expand_macros(start);
#ifdef EVAL_DEBUG
		fprintf(stderr, "eval_expand_macros('%s') = '%s'\n", free_me, ret);
#endif
	}
	snapshot_unblue_used(snapshot);
	snapshot_free(snapshot);

	return start;
}

static char *eval_macro(macro *m, char *start, char *at)
{
	char *r;
	if(m->blue)
		return start;

	m->use_cnt++;

	m->blue++;
	r = eval_macro_double_eval(m, start, at);
	m->blue--;
	return r;
}

char *eval_expand_macros(char *line)
{
	size_t i;

	for(i = 0; line[i]; i++){
		char *end, save;
		macro *m;
		{
			char *start = word_find_any(line + i);
			if(!start)
				break;
			i = start - line;
		}
		end = word_end(line + i);
		save = *end, *end = '\0';
		m = macro_find(line + i);
		*end = save;

		if(m){
			line = eval_macro(m, line, line + i);

			while(iswordpart(line[i]))
				i++;

		}else{
			/* skip this word */
			i = end - line; /* i incremented by loop */
		}
	}

	return line;
}
