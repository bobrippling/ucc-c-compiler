#ifndef SYM_H
#define SYM_H

struct sym
{
	decl *decl;
	int offset; /* stack offset */
	enum
	{
		sym_auto,
		sym_arg
	} type;

	sym  *next;
};

struct symtable
{
	symtable *parent;
	sym *first;
};

symtable *symtab_new();
sym *symtab_add( symtable *, decl *);
void symtab_push(symtable *, symtable *);
sym  *symtab_search(symtable *, const char *);

#endif
