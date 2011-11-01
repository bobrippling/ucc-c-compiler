#ifndef SYM_H
#define SYM_H

struct sym
{
	int offset; /* stack offset */
	enum sym_type
	{
		sym_global,
		sym_auto,
		sym_str,
		sym_arg,
		sym_func
	} type;

	char *str_lbl;

	decl *decl;
};

struct symtable
{
	int auto_offset;
	symtable *parent;
	decl **decls;
};

symtable *symtab_new();
symtable *symtab_child(symtable *);
symtable *symtab_grandparent(symtable *child);

sym  *symtab_add(   symtable *, decl *, enum sym_type);
sym  *symtab_search(symtable *, const char *);
sym  *symtab_has(   symtable *, decl *);
void  symtab_nest(symtable *parent, symtable **brat);

const char *sym_to_str(enum sym_type);

#endif
