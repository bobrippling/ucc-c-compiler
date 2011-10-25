#ifndef SYM_H
#define SYM_H

typedef struct symtable    symtable;
typedef struct sym         sym;

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

	decl *decl;
	char *str_lbl;

	sym *next
};

struct symtable
{
	int auto_offset;
	symtable *parent;
	decl **syms;
};

symtable *symtab_new();
symtable *symtab_child(symtable *);

sym *symtab_add( symtable *, decl *, enum sym_type);
void symtab_push(symtable *, symtable *);
sym  *symtab_search(symtable *, const char *, global **globals);
void symtab_nest(symtable *parent, symtable **brat);

const char *sym_to_str(enum sym_type);

#endif
