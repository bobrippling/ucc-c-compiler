#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../util/util.h"
#include "data_structs.h"
#include "../util/alloc.h"
#include "typedef.h"
#include "sym.h"

decl *typedef_find4(symtable *stab, const char *spel, decl *exclude, int descend)
{
	for(; stab; stab = stab->parent){
		decl **di;
		for(di = stab->typedefs; di && *di; di++){
			decl *d = *di;
			if(d != exclude && !strcmp(d->spel, spel))
				return d;
		}
		if(!descend)
			break;
	}

	return NULL;
}

decl *typedef_find(symtable *stab, const char *spel)
{
	return typedef_find4(stab, spel, NULL, 1);
}
