#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "tree.h"
#include "sym.h"
#include "alloc.h"
#include "macros.h"

sym *sym_new(decl *d)
{
	sym *p = umalloc(sizeof *p);
	p->decl = d;
	return p;
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

	new->next = tab->first;
	tab->first = new;

	new->type = t;

	return new;
}

void symtab_free(symtable *tab)
{
	sym *iter, *fme;

	for(iter = tab->first; iter; iter = iter->next, free(fme))
		fme = iter;

	free(tab);
}

sym *symtab_search(symtable *tab, const char *spel, global **globals)
{
	sym *s;

	for(; tab; tab = tab->parent)
		for(s = tab->first; s; s = s->next)
			/* if s->decl->spel is NULL, then it's a string lit. */
			if(s->decl->spel && !strcmp(spel, s->decl->spel))
				return s;

	for(; *globals; globals++){
		decl *d = (*globals)->isfunc ? (*globals)->ptr.f->func_decl : (*globals)->ptr.d;
		if(!strcmp(spel, d->spel)){
			s = sym_new(d);
			s->offset = 0;
			s->type = sym_global;
			return s;
		}
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
