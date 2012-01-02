#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "macro.h"
#include "parse.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "str.h"


macro **macros = NULL;
char **dirs = NULL;

void macro_add_dir(char *d)
{
	dynarray_add((void ***)&dirs, d);
}

macro *macro_find(const char *sp)
{
	macro **i;
	for(i = macros; i && *i; i++)
		if(!strcmp((*i)->nam, sp))
			return *i;
	return 0;
}

void macro_add(const char *nam, const char *val)
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
	char *pos;
	macro **iter;

	if(should_noop())
		**pline = '\0';

	if(!**pline)
		return; /* optimise for empty lines also */

	for(iter = macros; iter && *iter; iter++){
		macro *m = *iter;

		pos = *pline;
		DEBUG(DEBUG_VERB, "word_find(\"%s\", \"%s\")\n", pos, m->nam);

		while((pos = word_find(pos, m->nam))){
			int posidx = pos - *pline;

			DEBUG(DEBUG_VERB, "word_replace(line=\"%s\", pos=\"%s\", nam=\"%s\", val=\"%s\")\n",
					*pline, pos, m->nam, m->val);

			*pline = word_replace(*pline, pos, m->nam, m->val);
			pos = *pline + posidx + strlen(m->val);

			iter = macros;
		}
	}
}
