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
#include "typedef.h"
#include "sue.h"
#include "funcargs.h"

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

static sym *symtab_search2(symtable *tab, const void *item, int (*cmp)(const void *, decl *), int descend)
{
	decl **diter;

	for(diter = tab->decls; diter && *diter; diter++)
		if(cmp(item, *diter))
			return (*diter)->sym;

	if(tab->parent && (descend || tab->internal_nest))
		return symtab_search2(tab->parent, item, cmp, descend);

	return NULL;
}

static int spel_cmp(const void *test, decl *item)
{
	char *sp = item->spel;
	return sp && item->sym && !strcmp(test, sp);
}

sym *symtab_search(symtable *tab, const char *spel)
{
	if(!spel)
		ICE("symtab_search() with NULL spel");
	return symtab_search2(tab, spel, spel_cmp, 1);
}

static int decl_cmp(const void *test, decl *item)
{
	return (const decl *)test == item;
}

sym *symtab_has(symtable *tab, decl *d)
{
	if(!d)
		ICE("symtab_has() with NULL decl");
	return symtab_search2(tab, d, decl_cmp, 1);
}

void symtab_add_args(symtable *stab, funcargs *fargs, const char *func_spel)
{
	int nargs, i;

	if(fargs->arglist){
		for(nargs = 0; fargs->arglist[nargs]; nargs++);

		/* add args backwards, since we push them onto the stack backwards - still need to do this here? */
		for(i = nargs - 1; i >= 0; i--){
			if(!fargs->arglist[i]->spel){
				DIE_AT(&fargs->where, "function \"%s\" has unnamed arguments", func_spel);
			}else{
				sym_new_stab(stab, fargs->arglist[i], sym_arg);
			}
		}
	}
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
