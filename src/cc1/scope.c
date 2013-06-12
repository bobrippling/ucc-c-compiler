#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../util/util.h"
#include "data_structs.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "scope.h"
#include "sym.h"

decl *scope_find(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		decl **di;

		for(di = stab->decls; di && *di; di++){
			decl *d = *di;
			if(!strcmp(d->spel, spel))
				return d;
		}
	}

	return NULL;
}

int typedef_visible(symtable *stab, const char *spel)
{
	decl *d = scope_find(stab, spel);
	return d && (d->store & STORE_MASK_STORE) == store_typedef;
}
