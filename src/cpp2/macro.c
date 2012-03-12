#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "macro.h"
#include "parse.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/util.h"
#include "str.h"
#include "main.h"

macro **macros = NULL;
char **lib_dirs = NULL;

void macro_add_dir(char *d)
{
	dynarray_add((void ***)&lib_dirs, d);
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
		fprintf(stderr, "cpp: warning: redefining \"%s\"\n", nam);
		free(m->nam);
		free(m->val);
	}else{
		m = umalloc(sizeof *m);
		dynarray_add((void ***)&macros, m);
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
		dynarray_rm((void **)macros, m);
		free(m);
	}
}

void filter_macro(char **pline)
{
	macro **iter;

	if(should_noop())
		**pline = '\0';

	if(!**pline)
		return; /* optimise for empty lines also */

	for(iter = macros; iter && *iter; iter++){
		macro *m = *iter;
		int did_replace = 0;

		if(m->type != MACRO){
			char *s, *last;
			char **args;
			char *open_b, *close_b;
			char *replace;
			char *line;
			char *pos;
			int i, nest;

			nest = 0;
			line = *pline;

relook:
			if(!(pos = word_find(line, m->nam)))
				continue;

			open_b  = strchr(pos, '(');
			close_b = nest_close_paren(open_b + 1);
			if(!open_b){
				/* ignore the macro */
				pos++;
				goto relook;
			}
			if(!close_b)
				die("no close paren for function-macro");

			*open_b  = '\0';
			*close_b = '\0';

			args = NULL;
			for(last = s = open_b + 1; *s; s++){
				switch(*s){
					case ',':
						if(nest > 0)
							break;
						if(s == last)
							die("no argument to function macro");
						*s = '\0';
						dynarray_add((void ***)&args, ustrdup(last));
						*s = ',';
						last = s + 1;
						break;
					case '(':
						nest++;
						break;
					case ')':
						nest--;
				}
			}
			if(last != s)
				dynarray_add((void ***)&args, ustrdup(last));

			{
				int got, exp;
				got = dynarray_count((void **)args);
				exp = dynarray_count((void **)m->args);

				if(m->type == VARIADIC ? got <= exp : got != exp){
					if(debug)
						for(i = 0; i < got; i++)
							fprintf(stderr, "args[%d] = \"%s\"\n", i, args[i]);

					die("wrong number of args to function macro \"%s\", got %d, expected %d%s%s%s",
							m->nam, got, exp,
							debug ? " (" : "",
							debug ? line : "",
							debug ? ")"  : ""
							);
				}
			}

			if(args)
				for(i = 0; args[i]; i++)
					str_trim(args[i]);

			replace = ustrdup(m->val);

			/* replace #x with the quote of arg x */
			for(s = strchr(replace, '#'); s; s = strchr(last, '#')){
				char *arg_target;

				if(s[1] == '#'){
					/* x ## y */
					ICE("TODO: pasting");
				}

				arg_target = word_dup(s + 1);
				last = s;

				for(i = 0; m->args[i]; i++){
					if(!strcmp(m->args[i], arg_target)){
						char *finish = s + 1 + strlen(arg_target);
						char *quoted = str_quote(args[i]);
						int offset;

						offset = (s - replace) + strlen(quoted);

						replace = str_replace(replace, s, finish, quoted);

						free(quoted);

						last = replace + offset;
						break;
					}
				}

				free(arg_target);
			}

			if(args){
				i = 0;

				if(m->args)
					for(; m->args[i]; i++)
						word_replace_g(&replace, m->args[i], args[i]);

				if(m->type == VARIADIC){
					char *rest = str_join(args + i, ", ");

					word_replace_g(&replace, "__VA_ARGS__", rest);
					free(rest);
				}

				dynarray_free((void ***)&args, free);
			}

			*pline = str_replace(*pline, pos, close_b + 1, replace);

			free(replace);

			did_replace = 1;
		}else{
			int is_counter;
			static int counter = 0; /* __COUNTER__ */
			char *val;
			int fval = 0;

			if(m->val){
				val = m->val;
			}else{
				extern const char *current_fname;
				extern int current_line;

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

			if(is_counter && did_replace)
				counter++;

			if(fval)
				free(val);
		}

		if(did_replace)
			iter = macros;
	}
}
