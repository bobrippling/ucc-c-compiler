#ifndef SYM_H
#define SYM_H

struct sym
{
	decl *decl;
	int offset; /* stack offset */
	enum sym_type
	{
		sym_global,
		sym_auto,
		sym_str,
		sym_arg,
		sym_func
	} type;

	sym  *next;

	char *str_lbl;
};

struct symtable
{
	int auto_offset;
	symtable *parent;
	sym *first;
};

symtable *symtab_new();
symtable *symtab_child(symtable *);

sym *symtab_add( symtable *, decl *, enum sym_type);
void symtab_push(symtable *, symtable *);
sym  *symtab_search(symtable *, const char *, global **globals);
void symtab_nest(symtable *parent, symtable **brat);

const char *sym_to_str(enum sym_type);

#endif
