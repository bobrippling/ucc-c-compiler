#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "sym.h"
#include "../util/alloc.h"
#include "macros.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "sue.h"
#include "funcargs.h"
#include "label.h"

sym *sym_new(decl *d, enum sym_type t)
{
	sym *s = umalloc(sizeof *s);
	UCC_ASSERT(!d->sym, "%s already has a sym", d->spel);
	s->decl = d;
	d->sym  = s;
	s->type = t;
	return s;
}

sym *sym_new_stab(symtable *stab, decl *d, enum sym_type t)
{
	sym *s = sym_new(d, t);
	dynarray_add(&stab->decls, d);
	return s;
}

void symtab_rm_parent(symtable *child)
{
	dynarray_rm(child->parent->children, child);
	child->parent = NULL;
}

void symtab_set_parent(symtable *child, symtable *parent)
{
	if(child->parent)
		symtab_rm_parent(child);
	child->parent = parent;
	dynarray_add(&parent->children, child);
}

symtable *symtab_new(symtable *parent)
{
	symtable *p = umalloc(sizeof *p);
	if(parent)
		symtab_set_parent(p, parent);
	return p;
}

symtable_global *symtabg_new(void)
{
	return umalloc(sizeof *symtabg_new());
}

symtable *symtab_root(symtable *child)
{
	for(; child->parent; child = child->parent);
	return child;
}

symtable *symtab_func_root(symtable *stab)
{
	while(stab->parent && stab->parent->parent)
		stab = stab->parent;
	return stab;
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

decl *symtab_search_d(symtable *tab, const char *spel, symtable **pin)
{
	decl **const decls = tab->decls;
	int i;

	/* must search in reverse order - find the most
	 * recent decl first (e.g. function prototype propagation)
	 */
	for(i = dynarray_count(decls) - 1; i >= 0; i--){
		decl *d = decls[i];
		if(d->spel && !strcmp(spel, d->spel)){
			if(pin)
				*pin = tab;
			return d;
		}
	}

	if(tab->parent)
		return symtab_search_d(tab->parent, spel, pin);

	return NULL;

}

sym *symtab_search(symtable *tab, const char *sp)
{
	decl *d = symtab_search_d(tab, sp, NULL);
	/* d->sym may be null if it's not been assigned yet */
	return d ? d->sym : NULL;
}

int typedef_visible(symtable *stab, const char *spel)
{
	decl *d = symtab_search_d(stab, spel, NULL);
	return d && (d->store & STORE_MASK_STORE) == store_typedef;
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

static void label_init(symtable **stab)
{
	*stab = symtab_func_root(*stab);
	if((*stab)->labels)
		return;
	(*stab)->labels = dynmap_new((dynmap_cmp_f *)strcmp);
}

void symtab_label_add(symtable *stab, label *lbl)
{
	label_init(&stab);

	dynmap_set(char *, label *,
			symtab_func_root(stab)->labels,
			lbl->spel, lbl);
}

label *symtab_label_find(symtable *stab, char *spel, where *w)
{
	label *lbl;

	stab = symtab_func_root(stab);

	lbl = stab->labels
		? dynmap_get(char *, label *,
		    stab->labels, spel)
		: NULL;

	if(!lbl){
		/* forward decl */
		lbl = label_new(w, spel, 0);
		symtab_label_add(stab, lbl);
	}

	return lbl;
}
