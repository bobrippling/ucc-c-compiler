#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "sym.h"
#include "../util/alloc.h"
#include "macros.h"

sym *sym_new(decl *d)
{
	sym *s = umalloc(sizeof *s);
	s->decl = d;
	d->sym  = s;
	return s;
}

symtable *symtab_new()
{
	symtable *p = umalloc(sizeof *p);
	return p;
}

symtable *symtab_child(symtable *paren)
{
	symtable *ret = symtab_new();
	ret->parent = paren;
	return ret;
}

symtable *symtab_grandparent(symtable *child)
{
	for(; child->parent; child = child->parent);
	return child;
}

void symtab_nest(symtable *parent, symtable **brat)
{
	if(*brat){
		if((*brat)->parent)
			fprintf(stderr, "ICW: code symtable parent already set\n");
		(*brat)->parent = parent;
	}else{
		*brat = symtab_child(parent);
	}
}

sym *symtab_add(symtable *tab, decl *d, enum sym_type t)
{
	sym *new;

	new = sym_new(d);
	new->type = t;

	dynarray_add((void ***)&tab->decls, d);

	return new;
}

sym *symtab_search(symtable *tab, const char *spel)
{
	for(; tab; tab = tab->parent){
		decl **diter;
		for(diter = tab->decls; diter && *diter; diter++)
			/* if diter->decl->spel is NULL, then it's a string lit. */
			if((*diter)->spel && !strcmp(spel, (*diter)->spel))
				return (*diter)->sym;
	}

	return NULL;
}

const char *sym_to_str(enum sym_type t)
{
	switch(t){
		CASE_STR(sym_auto);
		CASE_STR(sym_arg);
		CASE_STR(sym_str);
		CASE_STR(sym_func);
		CASE_STR(sym_global);
	}
	return NULL;
}
