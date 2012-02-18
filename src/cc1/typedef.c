#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../util/util.h"
#include "tree.h"
#include "../util/alloc.h"
#include "typedef.h"
#include "sym.h"

decl *typedef_find(symtable *tab, const char *spel)
{
	UCC_ASSERT(spel, "NULL spell in %s", __func__);

	for(; tab; tab = tab->parent){
		decl **di;
		for(di = tab->typedefs; di; di++){
			decl *d = *di;
			if(!strcmp(d->spel, spel))
				return d;
		}
	}

	return NULL;
}
