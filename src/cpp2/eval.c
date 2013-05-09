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

static char *eval_macro_1(macro *m, char *args_str)
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

	/* XXX: progress here */
	{
		int i;
		for(i = 0; args[i]; i++)
			fprintf(stderr, "arg[%d] = \"%s\"\n", i, args[i]);
	}

	ICE("TODO");
#if 0
tok_fin:
	{
		int got, exp;
		got = dynarray_count(args);
		exp = dynarray_count(m->args);

		if(m->type == VARIADIC ? got <= exp : got != exp){
			if(option_debug){
				int i;
				for(i = 0; i < got; i++)
					fprintf(stderr, "args[%d] = \"%s\"\n", i, args[i]);
			}

			CPP_DIE("wrong number of args to function macro \"%s\", got %d, expected %d",
					m->nam, got, exp);
		}
	}

	if(args){
		int i;
		for(i = 0; args[i]; i++)
			str_trim(args[i]);
	}

	replace = ustrdup(m->val);

	/* replace #x with the quote of arg x */
	for(s = strchr(replace, '#'); s; s = strchr(last, '#')){
		char *const hash = s++;
		char *arg_target;
		int paste = 0; /* x ## y ? */
		int found;

		if(*s == '#'){
			/* x ## y */
			paste = 1;
			s++;
		}

		while(isspace(*s))
			s++;

		arg_target = word_dup(s);
		found = 0;

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

		for(i = 0; m->args && !found && m->args[i]; i++){
			if(!strcmp(m->args[i], arg_target)){
				char *finish = s + strlen(arg_target);
				char *quoted = str_quote(args[i]);
				const int offset = (s - replace) + strlen(quoted);

				replace = str_replace(replace, s - 1, finish, quoted);

				free(quoted);

				last = replace + offset;

				found = 1;
				break;
			}
		}

		if(!found)
			CPP_DIE("can't paste non-existent argument %s", arg_target);

		free(arg_target);
	}

	if(args){
		i = 0;

		if(m->args)
			for(; m->args[i]; i++)
				word_replace_g(&replace, m->args[i], args[i]);

		if(m->type == VARIADIC){
			char *rest = str_join(args + i, ", ");

			word_replace_g(&replace, VA_ARGS_STR, rest);
			free(rest);
		}

		dynarray_free(&args, free);
	}

	{
		int diff = close_b - *pline + 1;

		*pline = str_replace(*pline, pos, close_b + 1, replace);

		substitute_here = *pline + diff;
	}

	free(replace);

	did_replace = 1;
#endif
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

		ret = word_replace(start, at, strlen(m->nam), m->val);
		if(ret != start)
			free(start);

		if(free_val)
			free(val);

		return ret;

	}else{
		char *open_b, *close_b;

		for(open_b = at + strlen(m->nam); isspace(*open_b); open_b++);

		if(*open_b != '(')
			return start; /* not an invocation */

		close_b = strchr_nest(open_b + 1, ')');
		if(!close_b)
			CPP_DIE("unterminated function-macro '%s'", m->nam);

		{
			char *all_args = ustrdup2(open_b + 1, close_b);
			char *eval_d = eval_macro_1(m, all_args);
			char *ret = str_replace(start, at, close_b, eval_d);

			free(all_args);
			free(eval_d);

			if(ret != start)
				free(start);

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
