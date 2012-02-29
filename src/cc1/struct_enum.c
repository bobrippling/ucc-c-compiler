#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "data_structs.h"
#include "struct.h"
#include "enum.h"
#include "struct_enum.h"
#include "cc1.h"

void st_en_set_spel(char **dest, char *spel, const char *desc)
{
	if(spel){
		*dest = spel;
	}else{
		*dest = umalloc(32);
		snprintf(*dest, 32, "<anon %s %p>", desc, (void *)dest);
	}
}

void st_en_lookup(void **save_to, int *incomplete, decl *d, symtable *stab, void *(*lookup)(symtable *, const char *), int is_struct)
{
	const char *const mode = is_struct ? "struct" : "enum";

	if(!*save_to){
		UCC_ASSERT(d->type->spel, "%s lookup: no %s spel (decl %s)", mode, mode, d->spel);
		if((*incomplete = !(*save_to = lookup(stab, d->type->spel))))
			cc1_warn_at(&d->type->where, 0, WARN_INCOMPLETE_USE, "using incomplete type %s %s", mode, d->type->spel);
	}else if(!d->type->spel){
		/* get the anon enum name */
		d->type->spel = is_struct ? d->type->struc->spel : d->type->enu->spel;
	}
}
