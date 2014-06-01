#ifndef SYM_H
#define SYM_H

#include "decl.h"
#include "../util/dynmap.h"

typedef struct sym sym;
struct sym
{
	union
	{
		int arg_offset;
		unsigned stack_pos;
	} loc;

	enum sym_type
	{
		sym_global, /* externs are sym_global */
		sym_local,
		sym_arg
	} type;

	decl *decl;
	type *owning_func; /* only for sym_arg */

	/* static analysis */
	int nreads, nwrites;
};

typedef struct static_assert static_assert;
struct static_assert
{
	struct expr *e;
	char *s;
	struct symtable *scope;
	int checked;
};

typedef struct symtable symtable;
struct symtable
{
	where where;

	int auto_total_size;
	unsigned folded : 1, laidout : 1;
	unsigned internal_nest : 1, are_params : 1;
	/*
	 * { int i; 5; int j; }
	 * j's symtab is internally represented like:
	 * { int i; 5; { int j; } }
	 *
	 * internal_nest marks if it is so, for duplicate checking
	 */

	decl *in_func; /* for r/w checks on args and return-type checks */

	char *lbl_begin, *lbl_end; /* for debug - lexical block */

	symtable *parent, **children;

	struct struct_union_enum_st **sues;

	/* identifiers and typedefs */
	decl **decls;

	/* char * => label * */
	struct dynmap *labels;

	static_assert **static_asserts;
};

typedef struct symtable_gasm symtable_gasm;
struct symtable_gasm
{
	decl *before; /* the decl this occurs before - NULL if last */
	char *asm_str;
};

typedef struct symtable_global symtable_global;
struct symtable_global
{
	symtable stab; /* ABI compatible with struct symtable */
	symtable_gasm **gasms;
	dynmap *literals;
	dynmap *unrecog_attrs;
};

sym *sym_new(decl *d, enum sym_type t);
sym *sym_new_stab(symtable *, decl *d, enum sym_type t);

symtable_global *symtabg_new(where *);

symtable *symtab_new(symtable *parent, where *w);
void      symtab_set_parent(symtable *child, symtable *parent);
void      symtab_rm_parent( symtable *child);

symtable *symtab_root(symtable *child);
symtable *symtab_func_root(symtable *stab);
#define symtab_func(st) symtab_func_root(st)->in_func
symtable_global *symtab_global(symtable *);

int symtab_nested_internal(symtable *parent, symtable *nest);
int symtable_is_parent(symtable *parent, symtable *nest);

#define symtab_add_to_scope(scope, d) \
	dynarray_add(&(scope)->decls, (d))

sym  *symtab_search(symtable *, const char *);
decl *symtab_search_d(symtable *, const char *, symtable **pin);

const char *sym_to_str(enum sym_type);

#define sym_free(s) free(s)

/* labels */
struct label *symtab_label_find_or_new(symtable *, char *, where *);
void symtab_label_add(symtable *, struct label *);

#endif
