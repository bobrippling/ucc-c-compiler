#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "tree.h"
#include "sym.h"
#include "alloc.h"

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

void symtab_add(symtable *tab, decl *d)
{
	sym *new;

	new = sym_new(d);

	new->next = tab->first;
	tab->first = new;
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
			if(!strcmp(s, sym->decl->spel))
				return sym;

	return NULL;
}
