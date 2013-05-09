#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "macro.h"
#include "parse.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/util.h"
#include "str.h"
#include "main.h"

#define VA_ARGS_STR "__VA_ARGS__"

macro **macros = NULL;
char **lib_dirs = NULL;

void macro_add_dir(char *d)
{
	dynarray_add(&lib_dirs, d);
}

macro *macro_find(const char *sp)
{
	macro **i;
	for(i = macros; i && *i; i++)
		if(!strcmp((*i)->nam, sp))
			return *i;
	return NULL;
}

macro *macro_add(const char *nam, const char *val)
{
	macro *m;

	m = macro_find(nam);

	if(m){
		CPP_WARN("cpp: warning: redefining \"%s\"", nam);
		free(m->nam);
		free(m->val);
	}else{
		m = umalloc(sizeof *m);
		dynarray_add(&macros, m);
	}

	m->nam = ustrdup(nam);
	m->val = val ? ustrdup(val) : NULL;
	m->type = MACRO;

	DEBUG(DEBUG_NORM, "macro_add(\"%s\", \"%s\")\n", nam, val);

	return m;
}

macro *macro_add_func(const char *nam, const char *val, char **args, int variadic)
{
	macro *m  = macro_add(nam, val);
	m->args = args;
	m->type = variadic ? VARIADIC : FUNC;
	return m;
}

void macro_remove(const char *nam)
{
	macro *m = macro_find(nam);

	if(m){
		free(m->nam);
		free(m->val);
		dynarray_rm(macros, m);
		free(m);
	}
}

static char **split_args(char *open_b, char *close_b)
{
	char **arg_separators = NULL;
	char *arg;

	arg = open_b;

	do{
		dynarray_add(&arg_separators, arg);
		arg = strchr_nest(arg, ',');
	}while(arg && arg < close_b);

	dynarray_add(&arg_separators, close_b);

	{
		char **args = NULL;
		size_t i;

		for(i = 0; arg_separators[i + 1]; i++){
			char *from = arg_separators[i] + 1;
			char *to   = arg_separators[i + 1] - 1;

			if(from < to)
				arg = ustrdup2(from, to);
			else
				arg = ustrdup("");

			dynarray_add(&args, arg);
		}

		dynarray_free(char **, &arg_separators, NULL);

		return args;
	}
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

		if(free_val)
			free(val);

		return ret;

	}else{
		char *open_b, *close_b;
		char **args;

		for(open_b = at + strlen(m->nam); isspace(*open_b); open_b++);

		if(*open_b != '(')
			return start; /* not an invocation */

		close_b = strchr_nest(open_b + 1, ')');
		if(!close_b)
			CPP_DIE("unterminated function-macro '%s'", m->nam);

		args = split_args(open_b, close_b);

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

char *filter_macro(char *line)
{
	size_t i;

	if(should_noop()){
		*line = '\0';
		return line;
	}

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
