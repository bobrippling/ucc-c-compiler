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
	return 0;
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
	m->val = ustrdup(val);

	DEBUG(DEBUG_NORM, "macro_add(\"%s\", \"%s\")\n", nam, val);

	return m;
}

macro *macro_add_func(const char *nam, const char *val, char **args)
{
	macro *m  = macro_add(nam, val);
	m->args = args;
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

		if(m->args){
			char *s, *last;
			char **args;
			char *open_b, *close_b; /* TODO: nesting */
			char *replace;
			char *line;
			char *pos;
			int i;

			line = *pline;

relook:
			if(!(pos = word_find(line, m->nam)))
				continue;

			open_b  = strchr(pos, '(');
			close_b = strchr(open_b + 1, ')');
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
				/* FIXME: tokenise and fix in general */
				if(*s == ','){
					if(s == last)
						die("no argument to function macro");
					*s = '\0';
					dynarray_add((void ***)&args, ustrdup(last));
					*s = ',';
					last = s + 1;
				}
			}

			{
				int got, exp;
				got = dynarray_count((void **)args);
				exp = dynarray_count((void **)m->args);
				if(got != exp)
					die("wrong number of args to function macro \"%s\", got %d, expected %d%s%s%s",
							m->nam, got, exp,
							debug ? " (" : "",
							debug ? line : "",
							debug ? ")"  : ""
							);
			}

			replace = ustrdup(m->val);

			for(i = 0; args[i]; i++)
				word_replace_g(&replace, m->args[i], args[i]);

			*pline = str_replace(*pline, open_b, close_b, replace);

			free(replace);

			did_replace = 1;
		}else{
			did_replace = word_replace_g(pline, m->nam, m->val);
		}

		if(did_replace)
			iter = macros;
	}
}
