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

void st_en_un_set_spel(char **dest, char *spel, const char *desc)
{
	if(spel){
		*dest = spel;
	}else{
		*dest = umalloc(32);
		snprintf(*dest, 32, "<anon %s %p>", desc, (void *)dest);
	}
}

static void st_en_lookup(void **save_to, int *incomplete,
		decl *d, symtable *stab,
		void *(*lookup)(symtable *, const char *),
		int is_struct_or_union)
{
	const char *const mode = is_struct_or_union ? "struct" : "enum";

	if(!*save_to){
		UCC_ASSERT(d->type->spel, "%s lookup: no %s spel (decl %s)", mode, mode, d->spel);
		if((*incomplete = !(*save_to = lookup(stab, d->type->spel))))
			cc1_warn_at(&d->type->where, 0, WARN_INCOMPLETE_USE, "using incomplete type %s %s", mode, d->type->spel);
	}else{
		*incomplete = 0;

		if(!d->type->spel){
			/* get the anon enum name */
			d->type->spel = is_struct_or_union ? d->type->struct_union->spel : d->type->enu->spel;
		}
	}
}


void st_en_lookup_chk(decl *d, symtable *stab)
{
	int incomplete;

	if(d->type->primitive == type_enum){
		st_en_lookup((void **)&d->type->enu,
				&incomplete, d, stab,
				(void *(*)(struct symtable *, const char *))enum_find,
				0);
	}else{
		st_en_lookup((void **)&d->type->struct_union,
				&incomplete, d, stab,
				(void *(*)(struct symtable *, const char *))struct_union_find,
				1);

		/* only do this check if we actually found something */
		/*                                         not-not, since the bit-field is :1 */
		if(d->type->struct_union && !!(d->type->primitive == type_union) != !!d->type->struct_union->is_union)
			die_at(&d->where, "unions are not structs");
	}

	if(incomplete && !decl_ptr_depth(d))
		die_at(&d->where, "use of %s%s%s",
				type_to_str(d->type),
				d->spel ? " " : "",
				d->spel ? d->spel : "");
}
