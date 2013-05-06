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

void filter_macro(char **pline)
{
	macro **iter;
	char *substitute_here;

	if(should_noop())
		**pline = '\0';

	if(!**pline)
		return; /* optimise for empty lines also */

	for(iter = macros; iter && *iter; iter++)
		(*iter)->used_in_loop = 0;

	substitute_here = *pline;

	for(iter = macros; iter && *iter; iter++){
		macro *m = *iter;
		int did_replace = 0;

		if(m->used_in_loop)
			continue;

		if(m->type != MACRO){
			char *s, *last;
			char **args;
			char *open_b, *close_b;
			char *replace;
			char *pos;
			int i, nest;

			nest = 0;

			if(!(pos = word_find(substitute_here, m->nam)))
				continue;

relook:
			open_b  = strchr(pos, '(');
			if(!open_b){
				/* ignore the macro */
				pos++;
				goto relook;
			}
			close_b = nest_close_paren(open_b + 1);
			if(!close_b)
				CPP_DIE("no close paren for function-macro '%s'", m->nam);

			*open_b  = '\0';
			*close_b = '\0';

			args = NULL;
			for(last = s = open_b + 1; ; s++){
				switch(*s){
					case ',':
						if(nest == 0){
							*s = '\0'; {
								dynarray_add(&args, ustrdup(last));
							} *s = ',';
							last = s + 1;
						}
						break;

					case '\0':
						if(s > last || (args && s == last)) /* args - otherwise it's () */
							dynarray_add(&args, ustrdup(last));
						goto tok_fin;

					case '(':
						nest++;
						break;

					case ')':
						nest--;
						break;
				}
			}

tok_fin:
			{
				int got, exp;
				got = dynarray_count(args);
				exp = dynarray_count(m->args);

				if(m->type == VARIADIC ? got <= exp : got != exp){
					if(option_debug)
						for(i = 0; i < got; i++)
							fprintf(stderr, "args[%d] = \"%s\"\n", i, args[i]);

					CPP_DIE("wrong number of args to function macro \"%s\", got %d, expected %d%s%s%s",
							m->nam, got, exp,
							option_debug ? " (" : "",
							option_debug ? *pline : "",
							option_debug ? ")"  : ""
							);
				}
			}

			if(args)
				for(i = 0; args[i]; i++)
					str_trim(args[i]);

			replace = ustrdup(m->val);

			/* replace #x with the quote of arg x */
			for(s = strchr(replace, '#'); s; s = strchr(last, '#')){
				char *const hash = s++;
				char *arg_target;
				int found;

				if(*s == '#'){
					/* x ## y */
					ICE("TODO: pasting");
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
		}else{
			int is_counter = 0;
			static int counter = 0; /* __COUNTER__ */
			char *val;
			int fval = 0;

			if(m->val){
				val = m->val;
			}else{
				fval = 1;

				if(!strcmp(m->nam, "__FILE__")){
					val = ustrprintf("\"%s\"", current_fname);
				}else if(!strcmp(m->nam, "__LINE__")){
					val = ustrprintf("%d", current_line);
				}else if((is_counter = !strcmp(m->nam, "__COUNTER__"))){
					val = ustrprintf("%d", counter);
				}else if(!strcmp(m->nam, "__DATE__")){
					fval = 0;
					val = cpp_date;
				}else if(!strcmp(m->nam, "__TIME__")){
					fval = 0;
					val = cpp_time;
				}else{
					ICE("invalid macro");
				}
			}

			did_replace = word_replace_g(pline, m->nam, val);

			substitute_here = strstr(*pline, m->nam);
			if(!substitute_here)
				substitute_here = *pline;
			else
				substitute_here++;

			if(did_replace)
				m->used_in_loop = 1;

			if(is_counter && did_replace)
				counter++;

			if(fval)
				free(val);
		}

		if(did_replace)
			iter = macros;
	}
}
