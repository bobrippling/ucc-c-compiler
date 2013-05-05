#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../util/util.h"
#include "data_structs.h"
#include "../util/alloc.h"
#include "typedef.h"
#include "sym.h"

static decl *scope_find4(
		symtable *stab,
		const char *spel,
		int *pdescended,
		decl *exclude)
{
	if(pdescended)
		*pdescended = 0;

	for(; stab; stab = stab->parent){
		decl **di;

		for(di = stab->decls; di && *di; di++){
			decl *d = *di;
			if(d != exclude && !strcmp(d->spel, spel))
				return d;
		}

		if(pdescended)
			*pdescended = 1;
	}

	return NULL;
}

decl *typedef_find_descended_exclude(
		symtable *stab, const char *spel, int *pdescended,
		decl *exclude)
{
	decl *d = scope_find4(stab, spel, pdescended, exclude);
	return d && d->store == store_typedef ? d : NULL;
}

decl *scope_find(symtable *stab, const char *spel)
{
	return scope_find4(stab, spel, NULL, NULL);
}

decl *scope_find(symtable *, const char *spel);

int typedef_visible(symtable *stab, const char *spel)
{
	return !!typedef_find_descended_exclude(stab, spel, NULL, NULL);
}
