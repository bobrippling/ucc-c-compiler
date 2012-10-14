#ifndef SYM_H
#define SYM_H

struct sym
{
	int offset; /* stack offset */

	enum sym_type
	{
		sym_global, /* externs are sym_global */
		sym_local,
		sym_arg
	} type;

	decl *decl;
	decl *owning_func; /* only for sym_arg */

	/* static analysis */
	int nreads, nwrites;
};

typedef struct static_assert static_assert;

struct static_assert
{
	expr *e;
	char *s;
	symtable *scope;
};

struct symtable
{
	int auto_total_size;
	int internal_nest;
	/*
	 * { int i; 5; int j; }
	 * j's symtab is internally represented like:
	 * { int i; 5; { int j; } }
	 *
	 * this marks if it is so, for duplicate checking
	 */

	symtable *parent, **children;

	decl                 **decls;
	struct_union_enum_st **sues;
	decl                 **typedefs;

	static_assert        **static_asserts;
};

sym *sym_new(decl *d, enum sym_type t);

symtable *symtab_new(symtable *parent);
void      symtab_set_parent(symtable *child, symtable *parent);
void      symtab_rm_parent( symtable *child);

symtable *symtab_root(symtable *child);

#define SYMTAB_APPEND  0
#define SYMTAB_PREPEND 1

#define SYMTAB_NO_SYM   0
#define SYMTAB_WITH_SYM 1

#define SYMTAB_ADD(tab, decl, type) symtab_add(tab, decl, type, SYMTAB_WITH_SYM, SYMTAB_APPEND)

sym  *symtab_add(   symtable *, decl *, enum sym_type, int with_sym, int prepend);
sym  *symtab_search(symtable *, const char *);
sym  *symtab_has(   symtable *, decl *);
void  symtab_add_args(symtable *stab, funcargs *fargs, const char *sp, decl *d_func);

const char *sym_to_str(enum sym_type);

#define sym_free(s) free(s)

#endif
