#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

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

sym *symtab_search(symtable *tab, const char *s)
{
	sym *sym;

	for(; tab; tab = tab->parent)
		for(sym = tab->first; sym; sym = sym->next)
			/* if sym->decl->spel is NULL, then it's a string lit. */
			if(sym->decl->spel && !strcmp(s, sym->decl->spel))
				return sym;

	return NULL;
}

const char *sym_to_str(enum sym_type t)
{
	switch(t){
		CASE_STR(sym_auto);
		CASE_STR(sym_arg);
		CASE_STR(sym_str);
	}
	return NULL;
}
