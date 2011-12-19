#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../util/util.h"
#include "tree.h"
#include "../util/alloc.h"
#include "typedef.h"

decl *typedef_find(struct tdeftable *defs, const char *spel)
{
	tdef *td;

	for(; defs; defs = defs->parent)
		for(td = defs->first; td; td = td->next)
			if(!strcmp(td->decl->spel, spel))
				return td->decl;

	return NULL;
}

void typedef_add(struct tdeftable *defs, decl *d)
{
	ICE("todo");
}
