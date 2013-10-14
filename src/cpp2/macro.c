#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

#include "macro.h"
#include "main.h"
#include "preproc.h"

#define VA_ARGS_STR "__VA_ARGS__"

#define ITER_MACROS(m)                \
	macro **i, *m;                      \
	for(i = macros; i && (m = *i); i++)

macro **macros = NULL;

macro *macro_find(const char *sp)
{
	ITER_MACROS(m)
		if(!strcmp(m->nam, sp))
			return m;
	return NULL;
}

macro *macro_add(const char *nam, const char *val)
{
	macro *m;

	if(!strcmp(nam, DEFINED_STR))
		CPP_DIE("\"" DEFINED_STR "\" can't be used as a macro name");

	m = macro_find(nam);

	if(m){
		/* only warn if they're different */
		if(strcmp(val, m->val)){
			char buf[WHERE_BUF_SIZ];

			CPP_WARN(WREDEF, "cpp: warning: redefining \"%s\"\n"
					"%s: note: previous definition here",
					nam, where_str_r(buf, &m->where));
		}

		free(m->val);
	}else{
		m = umalloc(sizeof *m);
		dynarray_add(&macros, m);

		m->nam = ustrdup(nam);
	}

	where_current(&m->where);

	m->val = val ? ustrdup(val) : NULL;
	m->type = MACRO;

	return m;
}

macro *macro_add_func(const char *nam, const char *val, char **args, int variadic)
{
	macro *m  = macro_add(nam, val);
	if(m->args)
		dynarray_free(char **, &m->args, free);
	m->args = args;
	m->type = variadic ? VARIADIC : FUNC;
	return m;
}

int macro_remove(const char *nam)
{
	macro *m = macro_find(nam);

	if(m){
		free(m->nam);
		free(m->val);
		dynarray_free(char **, &m->args, free);
		dynarray_rm(macros, m);
		free(m);
		return 1;
	}
	return 0;
}

void macro_use(macro *m, int adj)
{
	m->use_cnt  += adj;
	m->use_dump += adj;
}

void macros_dump(void)
{
	ITER_MACROS(m){
		if(m->val){
			printf("#define %s", m->nam);
			switch(m->type){
				case FUNC:
				case VARIADIC:
				{
					char **arg;
					putchar('(');
					for(arg = m->args; arg && *arg; arg++)
						printf("%s%s", *arg, arg[1] ? ", " : "");
					if(m->type == VARIADIC)
						printf(", ...");
					putchar(')');
					case MACRO:
					break;
				}
			}
			printf(" %s\n", m->val);
		}
	}
}

void macros_stats(void)
{
	ITER_MACROS(m)
		printf("%s %d\n", m->nam, m->use_dump);
}
