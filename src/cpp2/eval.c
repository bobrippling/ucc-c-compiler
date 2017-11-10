#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "eval.h"

#include "../util/dynarray.h"
#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/str.h"

#include "tokenise.h"
#include "macro.h"
#include "str.h"
#include "main.h"
#include "snapshot.h"
#include "preproc.h"
#include "has.h"

/* for __has_include() */
#include "directive.h"
#include "include.h"

#define VA_ARGS_STR "__VA_ARGS__"

static char **split_func_args(char *args_str)
{
	token **tokens = tokenise(args_str);
	token **ti, **anchor = tokens;
	char **args = NULL;
	unsigned nest = 0;

	for(ti = tokens; ti && *ti; ti++){
		token *t = *ti;

		switch(t->tok){
			case TOKEN_COMMA:
				if(nest == 0){
					char *arg = tokens_join_n(anchor, ti - anchor);
					dynarray_add(&args, arg);
					anchor = ti + 1;
				}
				break;
			case TOKEN_OPEN_PAREN:
				nest++;
				break;
			case TOKEN_CLOSE_PAREN:
				nest--;
				break;
			default:
				break;
		}
	}

	/* need to account for a finishing comma */
	if(anchor != ti
	|| (ti > tokens && ti[-1]->tok == TOKEN_COMMA))
	{
		dynarray_add(&args, tokens_join_n(anchor, ti - anchor));
	}

	tokens_free(tokens);

	return args;
}

static char *find_arg(macro *m, char *word, char **args, int *alloced)
{
	*alloced = 0;
	if(!strcmp(word, VA_ARGS_STR)){
		if(args){
			size_t i = dynarray_count(m->args);
			/* if count(args) < i then args[i] is NULL,
			 * which str_join handles
			 */
			*alloced = 1;
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
		macro *m, token *this, char **args,
		int *alloced,
		int need_arg, const char *emsg)
{
	char *word;

	if(!this)
		CPP_DIE("nothing to %s after '#' character", emsg);
	if(this->tok != TOKEN_WORD)
		CPP_DIE("can't %s a none-word (%s)", emsg, token_str(this));

	word = find_arg(m, this->w, args, alloced); /* don't expand */
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

	if(!m->val){
		int ret;

		if(got != 1){
			CPP_DIE("too %s arguments to builtin macro \"%s\"",
					args ? "many" : "few", m->nam);
		}

		/* we don't want macro-substitution of the argument */
		ret = has_func(m->nam, args[0]);

		dynarray_free(char **, args, free);

		return ustrdup(ret ? "1" : "0");
	}


	if(m->type == VARIADIC ? got < exp : got != exp){
		if(got == 0 && exp == 1){
			/* special case - invoking with empty argument list
			 * e.g.
			 * #define F(x) ...
			 * F()
			 */
			CPP_WARN(WEMPTY_ARG,
					"empty argument list to single-argument macro \"%s\"",
					m->nam, args);
			dynarray_add(&args, ustrdup(""));

		}else{
			CPP_DIE("wrong number of args to function macro \"%s\", "
					"got %d, expected %s%d",
					m->nam, got, m->type == VARIADIC ? "at least " : "", exp);
		}
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
				{
					/* replace #arg with the quote of arg */
					/* # - don't eval */
					int alloced;
					char *w = noeval_hash(m, *++ti, args, &alloced, 1, "quote");
					w = str_quote(w, alloced);

					APPEND(this->had_whitespace, "%s", w);

					free(w);
					break;
				}

				case TOKEN_HASH_JOIN:
					/* replace a ## b with the join of both */
					CPP_DIE("## with no prior argument");

				case TOKEN_WORD:
				{
					int free_word = 0;
					char *word;

					if(ti[1] && ti[1]->tok == TOKEN_HASH_JOIN){
						/* word with ## - don't eval */
						word = noeval_hash(m, ti[0], args, &free_word, 0, "join");

						ti++;

						while(*ti && ti[0]->tok == TOKEN_HASH_JOIN){
							char *old = free_word ? word : (free_word = 1, ustrdup(word));
							char *neh;
							int free_neh;
							char *p;

							word = ustrprintf("%s%s", word,
									neh = noeval_hash(m, ti[1], args, &free_neh, 0, "join"));

							p = word;
							if(iswordpart(*p) || (*p && iswordpart(p[1]))){
								/* else we might have < ## < which gives << */
								for(; *p; p++)
									if(!iswordpart(*p)){
										CPP_WARN(WPASTE,
												"pasting \"%s\" and \"%s\" doesn't "
												"give a single token",
												old, neh);
										break;
									}
							}

							if(free_neh)
								free(neh);
							if(free_word)
								free(old);
							free_word = 1;

							ti += 2;
						}
						ti--;
					}else{
						/* word without # nor ## - we may eval */
						word = find_arg(m, this->w, args, &free_word);
						if(word != this->w){
							/* is an argument, eval.
							 * "... is replaced by the corresponding argument after all
							 * macros contained therein have been expanded..."
							 */

							if(!free_word){
								/* eval_expand_macros() (below) frees its argument
								 * if it expands, so we must own 'word' at this point */
								word = ustrdup(word);
								free_word = 1;
							}

							/* we don't blue paint arguments, e.g.
							 * define G(x) x+5
							 * define F(x) G(x)
							 * F(G(1))
							 * we end up with `replace = "G(x+5)`
							 * which we then want to double-expand
							 */
							{
								snapshot *snap = snapshot_take();

								word = eval_expand_macros(word);

								snapshot_restore_used(snap);
								snapshot_free(snap);
							}
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


		dynarray_free(char **, args, free);
		tokens_free(toks);
		return replace;
	}
#undef APPEND
}

static char *eval_macro_r(macro *m, char *start, char **pat)
{
	if(m->type == MACRO){
		static int counter = 0; /* __COUNTER__ */
		int free_val = 0;
		char *val;
		char *ret;
		size_t change;

		if(m->val){
			val = m->val;
		}else{
			free_val = 1;

			if(!strcmp(m->nam, "__FILE__")){
				char *q = str_quote(current_fname, 0);
				val = ustrprintf("%s", q);
				free(q);
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
			}else if(!strcmp(m->nam, "__TIMESTAMP__")){
				free_val = 0;
				val = cpp_timestamp;
			}else{
				ICE("invalid macro");
			}
		}

		change = *pat - start;
		ret = word_replace(start, *pat, strlen(m->nam), val);
		*pat = ret + change + strlen(val);

		if(free_val)
			free(val);

		return ret;

	}else{
		char *open_b, *close_b;

		open_b = str_spc_skip(*pat + strlen(m->nam));

		if(*open_b != '('){
			/* not an invocation - return and also knock down the use-count */
			macro_use(m, -1);
			CPP_WARN(WUNCALLED_FN, "ignoring non-function instance of %s", m->nam);
			return start;
		}

		close_b = strchr_nest(open_b, ')');
		if(!close_b)
			CPP_DIE("unterminated function-macro '%s'", m->nam);

		{
			char *all_args = ustrdup2(open_b + 1, close_b);
			char *eval_d = eval_func_macro(m, all_args);
			size_t change;

			free(all_args);

			change = *pat - start;
			start = str_replace(start, *pat, close_b + 1, eval_d);
			*pat = start + change + strlen(eval_d);

			free(eval_d);

			return start;
		}
	}
}

static char *eval_macro_double_eval(macro *m, char *start, char **pat)
{
	/* FIXME: need to snapshot *m too? i.e. snapshot one level higher */
	snapshot *snapshot = snapshot_take();
	size_t change;

	start = eval_macro_r(m, start, pat);
	change = *pat - start;

	/* mark any macros that changed as blue, to prevent re-evaluation */
	snapshot_take_post(snapshot);
	snapshot_blue_used(snapshot);

	/* double eval */
	start = eval_expand_macros(start);
	*pat = start + change;

	snapshot_unblue_used(snapshot);
	snapshot_free(snapshot);

	return start;
}

static char *eval_macro(macro *m, char *start, char **pat)
{
	char *r;
	if(m->blue){
		*pat += strlen(m->nam);
		return start;
	}

	macro_use(m, +1);

	m->blue++;
	r = eval_macro_double_eval(m, start, pat);
	m->blue--;
	return r;
}

char *eval_expand_macros(char *line)
{
	char *anchor = line;

	for(; *line; line++){
		char *end, save;
		macro *m;

		line = word_find_any(line);
		if(!line)
			break;

		end = word_end(line);

		if(*end == '"' && line == end - 1 && *line == 'L'){
			line = end;
			continue;
		}

		save = *end, *end = '\0';
		m = macro_find(line);
		*end = save;

		if(m){
			anchor = eval_macro(m, anchor, &line);

			switch(*line){
				case '"':
				case '\'':
				{
					/* skip quotes */
					char *p = str_quotefin2(line + 1, *line);
					UCC_ASSERT(p, "no matching quote on line '%s' @ '%s'", anchor, line);
					break;
				}
			}

		}else{
			/* skip this word */
			line = end;
		}
		if(!*line)
			break;
	}

	return anchor;
}

static char *eval_expand(char *w, const char *from, int eval(char *))
{
	char *entry;

	while((entry = word_find(w, from))){
		char *const entry_end = str_spc_skip(word_end(entry));
		char *closeparen;
		char buf[2];
		int replace;

		if((*entry_end != '('))
			CPP_DIE("open paren expected for \"%s\"", from);

		closeparen = strchr_nest(entry_end, ')');
		if(!closeparen)
			CPP_DIE("close paren expected for \"%s\"", from);

		*closeparen = '\0';

		replace = eval(entry_end+1);
		snprintf(buf, sizeof buf, "%d", replace);

		*closeparen = ')';

		w = str_replace(w, entry, closeparen + 1, buf);
	}

	return w;
}

static int defined_macro_find(char *ident)
{
	char *end;
	char save;
	int ret;

	ident = word_find_any(ident);
	end = word_end(ident);
	assert(end);
	save = *end;
	*end = '\0';

	ret = !!macro_find(ident);
	*end = save;
	return ret;
}

char *eval_expand_defined(char *w)
{
	return eval_expand(w, DEFINED_STR, defined_macro_find);
}

static int has_include(char *arg)
{
	int angle;
	const char *fname = include_parse(arg, &angle, 1);
	const char *curdir = cd_stack[dynarray_count(cd_stack) - 1];
	char *path;
	int sysh;
	FILE *f = include_fopen(curdir, fname, angle, &path, &sysh);
	int ret = 0;

	free(path);
	if(f){
		fclose(f);
		ret = 1;
	}

	return ret;
}

char *eval_expand_has_include(char *w)
{
	return eval_expand(w, HAS_INCLUDE_STR, has_include);
}
