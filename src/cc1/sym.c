#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "../util/util.h"
#include "sym.h"
#include "../util/alloc.h"
#include "macros.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "sue.h"
#include "funcargs.h"
#include "label.h"
#include "type_is.h"

static symtable *symtab_add_target(symtable *symtab)
{
	for(; symtab->transparent; symtab = symtab->parent);

	assert(symtab && "no non-transparent symtable");
	return symtab;
}

static void symtab_add_to_scope2(
		symtable *symtab, decl *d, int prepend)
{
	symtab = symtab_add_target(symtab);

	if(prepend)
		dynarray_prepend(&symtab->decls, d);
	else
		dynarray_add(&symtab->decls, d);
}

void symtab_add_to_scope(symtable *symtab, decl *d)
{
	symtab_add_to_scope2(symtab, d, 0);
}

void symtab_add_sue(symtable *symtab, struct_union_enum_st *sue)
{
	symtab = symtab_add_target(symtab);

	dynarray_add(&symtab->sues, sue);
}

sym *sym_new(decl *d, enum sym_type t)
{
	sym *s = umalloc(sizeof *s);
	UCC_ASSERT(!d->sym, "%s already has a sym", d->spel);
	s->decl = d;
	d->sym  = s;
	s->type = t;
	return s;
}

sym *sym_new_and_prepend_decl(symtable *stab, decl *d, enum sym_type t)
{
	sym *s = sym_new(d, t);
	symtab_add_to_scope2(stab, d, 1);
	return s;
}

void symtab_rm_parent(symtable *child)
{
	dynarray_rm(&child->parent->children, child);
	child->parent = NULL;
}

void symtab_set_parent(symtable *child, symtable *parent)
{
	if(child->parent)
		symtab_rm_parent(child);
	child->parent = parent;
	dynarray_add(&parent->children, child);
}

symtable *symtab_new(symtable *parent, where *w)
{
	symtable *p = umalloc(sizeof *p);
	UCC_ASSERT(parent, "no parent for symtable");
	symtab_set_parent(p, parent);
	memcpy_safe(&p->where, w);
	return p;
}

symtable *symtab_new_transparent(symtable *parent, where *w)
{
	symtable *p = symtab_new(parent, w);
	p->transparent = 1;
	return p;
}

symtable_global *symtabg_new(where *w)
{
	symtable_global *s = umalloc(sizeof *s);
	memcpy_safe(&s->stab.where, w);
	return s;
}

symtable *symtab_root(symtable *stab)
{
	for(; stab->parent; stab = stab->parent);
	return stab;
}

symtable *symtab_func_root(symtable *stab)
{
	while(stab->parent && stab->parent->parent)
		stab = stab->parent;
	return stab;
}

symtable_global *symtab_global(symtable *stab)
{
	return (symtable_global *)symtab_root(stab);
}

int symtab_nested_internal(symtable *parent, symtable *nest)
{
	while(nest && nest->internal_nest){
		if(nest->parent == parent)
			return 1;
		nest = nest->parent;
	}
	return 0;
}

int symtab_search(
		symtable *tab, const char *spel, decl *exclude, struct symtab_entry *ent)
{
	decl **decls;
	int i;

	if(!tab)
		return 0;
	decls = tab->decls;

	/* must search in reverse order - find the most
	 * recent decl first (e.g. function prototype propagation)
	 *
	 * at first glance this may appear to break enums:
	 * f()
	 * {
	 *   int a;
	 *   enum { a };
	 * }
	 *
	 * will find 'int a' first - but this is fine - the above case
	 * can't happen as it's a symbol collision
	 */
	for(i = dynarray_count(decls) - 1; i >= 0; i--){
		decl *d = decls[i];
		if(d != exclude && d->spel && !strcmp(spel, d->spel)){
			ent->type = SYMTAB_ENT_DECL;
			ent->bits.decl = d;
			ent->owning_symtab = tab;
			return 1;
		}
	}

	enum_member_search_nodescend(
			&ent->bits.enum_member.memb,
			&ent->bits.enum_member.sue,
			tab,
			spel);

	if(ent->bits.enum_member.memb){
		ent->type = SYMTAB_ENT_ENUM;
		ent->owning_symtab = tab;
		return 1;
	}

	return symtab_search(tab->parent, spel, exclude, ent);
}

const char *sym_to_str(enum sym_type t)
{
	switch(t){
		CASE_STR_PREFIX(sym, local);
		CASE_STR_PREFIX(sym, arg);
		CASE_STR_PREFIX(sym, global);
	}
	return NULL;
}

static void label_init(symtable *stab)
{
	if(stab->labels)
		return;
	stab->labels = dynmap_new(char *, strcmp, dynmap_strhash);
}

int symtab_label_add_local(symtable *stab, char *spel, where *loc)
{
	label *lbl, *prev;

	label_init(stab);

	lbl = dynmap_get(char *, label *, stab->labels, spel);
	if(lbl)
		return 0;

	lbl = label_new(loc, /*consumed*/spel, 0, stab);

	prev = dynmap_set(char *, label *, stab->labels, spel, lbl);
	assert(!prev);
	return 1;
}

void symtab_label_add(symtable *stab, label *lbl)
{
	label *prev;

	stab = symtab_func_root(stab);
	label_init(stab);

	prev = dynmap_set(char *, label *,
			symtab_func_root(stab)->labels,
			lbl->spel, lbl);

	assert(!prev);
}

label *symtab_label_find_or_new(symtable *const stab, char *spel, where *w)
{
	symtable *const func_root = symtab_func_root(stab), *siter = stab;
	label *lbl;

	for(; siter; siter = siter->parent){
		lbl = siter->labels
			? dynmap_get(char *, label *, siter->labels, spel)
			: NULL;

		if(lbl)
			break;
	}

	if(!lbl){
		/* forward decl */
		lbl = label_new(w, spel, 0, stab);

		/* already create new ones in func_root
		 * - local labels are created separately */
		symtab_label_add(func_root, lbl);
	}

	return lbl;
}

unsigned sym_hash(const sym *sym)
{
	return sym->type ^ (unsigned)(intptr_t)sym;
}

int symtab_is_transparent(symtable const *stab)
{
	/* symtable tables are the same if transparent
	 * or parent symtable is a parameter one */
	return stab->transparent
		|| (stab->parent && stab->parent->are_params);
}

unsigned symtab_decl_bytes(
		symtable *stab,
		unsigned const vla_cost,
		int array_only,
		int *const addr_taken)
{
	unsigned total = 0;
	symtable **si;
	decl **di;

	for(di = stab->decls; di && *di; di++){
		decl *d = *di;

		if((d->flags & DECL_FLAGS_ADDRESSED) && addr_taken)
			*addr_taken = 1;

		if(array_only && !type_is_array(d->ref))
			continue;

		if(type_is_variably_modified(d->ref))
			total += vla_cost;
		else
			total += decl_size(d);
	}

	for(si = stab->children; si && *si; si++)
		total += symtab_decl_bytes(*si, vla_cost, array_only, addr_taken);

	return total;
}

const struct out_val *sym_outval(sym *s)
{
	switch(s->type){
		case sym_global:
		case sym_arg:
			return s->out.val_single;

		case sym_local:
			if(s->out.stack.n == 0)
				return NULL;

			return s->out.stack.vals[s->out.stack.n - 1];
	}
	assert(0);
}

void sym_setoutval(sym *s, const struct out_val *v)
{
	switch(s->type){
		case sym_global: /* exactly one should be null */
			assert(!v ^ !s->out.val_single);
		case sym_arg: /* fine to overwrite - inline code takes care */
			s->out.val_single = v;
			return;

		case sym_local:
			if(v == NULL){
				s->out.stack.n--;
				(void)dynarray_pop(const struct out_val *, &s->out.stack.vals);
			}else{
				s->out.stack.n++;
				dynarray_add(&s->out.stack.vals, v);
			}
			return;
	}
	assert(0);
}
