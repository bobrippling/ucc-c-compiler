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

#define VA_ARGS_STR "__VA_ARGS__"

macro **macros = NULL;

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

	if(!strcmp(nam, DEFINED_STR))
		CPP_DIE("\"" DEFINED_STR "\" can't be used as a macro name");

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
