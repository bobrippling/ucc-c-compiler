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
	dynarray_prepend(&stab->decls, d);
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

decl *symtab_search_d_exclude(
		symtable *tab, const char *spel, symtable **pin,
		decl *exclude)
{
	decl **const decls = tab->decls;
	int i;

	/* must search in reverse order - find the most
	 * recent decl first (e.g. function prototype propagation)
	 */
	for(i = dynarray_count(decls) - 1; i >= 0; i--){
		decl *d = decls[i];
		if(d != exclude && d->spel && !strcmp(spel, d->spel)){
			if(pin)
				*pin = tab;
			return d;
		}
	}

	if(tab->parent)
		return symtab_search_d_exclude(tab->parent, spel, pin, exclude);

	return NULL;

}

decl *symtab_search_d(symtable *tab, const char *spel, symtable **pin)
{
	return symtab_search_d_exclude(tab, spel, pin, NULL);
}

sym *symtab_search(symtable *tab, const char *sp)
{
	decl *d = symtab_search_d(tab, sp, NULL);
	if(!d)
		return NULL;

	/* if it doesn't have a symbol, we haven't finished parsing yet */
	return d->sym;
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
	(*stab)->labels = dynmap_new(char *, strcmp, dynmap_strhash);
}

void symtab_label_add(symtable *stab, label *lbl)
{
	label *prev;

	label_init(&stab);

	prev = dynmap_set(char *, label *,
			symtab_func_root(stab)->labels,
			lbl->spel, lbl);

	assert(!prev);
}

label *symtab_label_find_or_new(symtable *const stab, char *spel, where *w)
{
	symtable *root;
	label *lbl;

	root = symtab_func_root(stab);

	lbl = root->labels
		? dynmap_get(char *, label *, root->labels, spel)
		: NULL;

	if(!lbl){
		/* forward decl */
		lbl = label_new(w, spel, 0, stab);
		symtab_label_add(root, lbl);
	}

	return lbl;
}

unsigned sym_hash(const sym *sym)
{
	return sym->type ^ (unsigned)(intptr_t)sym;
}
