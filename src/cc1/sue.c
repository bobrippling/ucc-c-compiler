#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "sue.h"
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

void sue_fold(decl *d, symtable *stab)
{
	(void)stab;

	if(d->type->primitive == type_enum){
		/* hi */
	}else{
		if(sue_incomplete(d->type->sue) && !decl_ptr_depth(d))
			die_at(&d->where, "use of %s%s%s",
					type_to_str(d->type),
					d->spel ?     " " : "",
					d->spel ? d->spel : "");
	}
}

void enum_vals_add(sue_member ***pmembers, char *sp, expr *e)
{
	sue_member *mem = umalloc(sizeof *mem);
	enum_member *emem = &mem->enum_member;
	if(!e)
		e = (expr *)-1;

	emem->spel = sp;
	emem->val  = e;

	dynarray_add((void ***)pmembers, emem);
}

int enum_nentries(struct_union_enum_st *e)
{
	int n = 0;
	sue_member **i;
	for(i = e->members; *i; i++, n++);
	return n;
}

int struct_union_size(struct_union_enum_st *st)
{
	sue_member **i;
	int total, max;

	total = max = 0;

	for(i = st->members; i && *i; i++){
		const int sz = decl_size(&(*i)->struct_member);
		total += sz;
		if(sz > max)
			max = sz;
	}

	return st->primitive == type_union ? max : total;
}

struct_union_enum_st *sue_find(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		struct_union_enum_st **i;

		for(i = stab->sues; i && *i; i++){
			struct_union_enum_st *st = *i;

			if(st->spel && !strcmp(st->spel, spel))
				return st;
		}
	}
	return NULL;
}

struct_union_enum_st *sue_add(symtable *const stab, char *spel, sue_member **members, enum type_primitive prim)
{
	struct_union_enum_st *sue;
	sue_member **iter;

	if(spel && (sue = sue_find(stab, spel))){
		char buf[DECL_STATIC_BUFSIZ];

		strcpy(buf, where_str(&sue->where));

		/* redef checks */
		if(sue->primitive != prim)
			die_at(NULL, "trying to redefine %s as %s (from %s)",
					sue_str(sue),
					type_primitive_to_str(prim),
					buf);

		if(members && !sue_incomplete(sue))
			die_at(NULL, "can't redefine %s %s's members (defined at %s)",
					sue_str(sue), sue->spel, buf);

	}else{
		sue = umalloc(sizeof *sue);
		sue->primitive = prim;

		where_new(&sue->where);
	}

	if(prim != type_enum)
		for(iter = members; iter && *iter; iter++){
			decl *d = &(*iter)->struct_member;
			if(d->init)
				die_at(&d->init->where, "%s member %s is initialised", sue_str(sue), d->spel);
		}

	sue->anon = !spel;

	st_en_un_set_spel(&sue->spel, spel, sue_str(sue));

	if(members)
		sue->members = members;

	dynarray_add((void ***)&stab->sues, sue);

	return sue;
}

sue_member *sue_member_find(struct_union_enum_st *sue, const char *spel, where *die_where)
{
	sue_member **mi;

	for(mi = sue->members; mi && *mi; mi++){
		char *sp;

		if(sue->primitive == type_enum){
			enum_member *m = &(*mi)->enum_member;
			sp = m->spel;
		}else{
			decl *d = &(*mi)->struct_member;
			sp = d->spel;
		}

		if(!strcmp(spel, sp))
			return *mi;
	}

	if(die_where)
		die_at(die_where, "%s %s has no member named \"%s\"", sue_str(sue), sue->spel, spel);

	return NULL;
}


enum_member *enum_member_search(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		struct_union_enum_st **i;

		for(i = stab->sues; i && *i; i++){
			struct_union_enum_st *e = *i;

			if(e->primitive == type_enum){
				enum_member *memb = (enum_member *)sue_member_find(e, spel, NULL);
				if(memb)
					return memb;
			}
		}
	}

	return NULL;
}

decl *struct_union_member_find(struct_union_enum_st *sue, const char *spel, where *die_where)
{
	return (decl *)sue_member_find(sue, spel, die_where);
}
