#ifndef SYM_H
#define SYM_H

#include "decl.h"
#include "../util/dynmap.h"

typedef struct sym sym;
struct sym
{
	union
	{
		struct
		{
			const struct out_val **vals; /* sym_{local,arg} */
			unsigned n;
		} stack;
		const struct out_val *val_single; /* sym_global */
	} out;
	long bp_offset;

	enum sym_type
	{
		sym_global, /* externs are sym_global */
		sym_local,
		sym_arg
	} type;

	decl *decl;
	type *owning_func; /* only for sym_arg */

	/* static analysis */
	unsigned nreads, nwrites;
};

typedef struct static_assert static_assert;
struct static_assert
{
	struct expr *e;
	char *s;
	struct symtable *scope;
	int checked;
};

struct out_dbg_lbl;

typedef struct symtable symtable;
struct symtable
{
	where where;

	unsigned mark : 1; /* used for scope checking */
	unsigned folded : 1, laidout : 1;
	unsigned internal_nest : 1, are_params : 1;
	unsigned transparent : 1;
	/*
	 * { int i; 5; int j; }
	 * j's symtab is internally represented like:
	 * { int i; 5; { int j; } }
	 *
	 * internal_nest marks if it is so, for duplicate checking
	 *
	 * transparent marks C99 for-statements creating a new scope
	 * - see symtab_new_transparent()
	 */

	decl *in_func; /* for r/w checks on args and return-type checks */

	/* for debug - lexical block */
	struct out_dbg_lbl *lbl_begin, *lbl_end;

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
	where where;
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
sym *sym_new_and_prepend_decl(symtable *, decl *d, enum sym_type t);

symtable_global *symtabg_new(where *);

const struct out_val *sym_outval(sym *);
void sym_setoutval(sym *, const struct out_val *);

/* symtab_new             - a new symbol table that decls are added to
 * symtab_new_transparent - a new symbol table that decls pass through to
 *                          higher scope - ensures separate symtables for
 *                          different statements, while keeping decls in
 *                          the same symtable
 */
symtable *symtab_new(symtable *parent, where *w);
symtable *symtab_new_transparent(symtable *parent, where *w);
void      symtab_set_parent(symtable *child, symtable *parent);
void      symtab_rm_parent( symtable *child);

symtable *symtab_root(symtable *child);
symtable *symtab_func_root(symtable *stab);
#define symtab_func(st) symtab_func_root(st)->in_func
symtable_global *symtab_global(symtable *);

int symtab_nested_internal(symtable *parent, symtable *nest);

unsigned symtab_decl_bytes(symtable *, unsigned const vla_cost);

void symtab_add_to_scope(symtable *, decl *);
void symtab_add_sue(symtable *, struct struct_union_enum_st *);
#define symtab_decls(stab) ((stab)->decls)

struct symtab_entry
{
	enum
	{
		SYMTAB_ENT_DECL,
		SYMTAB_ENT_ENUM
	} type;
	union
	{
		decl *decl;
		struct
		{
			struct struct_union_enum_st *sue;
			struct enum_member *memb;
		} enum_member;
	} bits;
	symtable *owning_symtab;
};

int symtab_search(symtable *, const char *, decl *exclude, struct symtab_entry *)
	ucc_nonnull((2, 4));

int symtab_is_transparent(symtable const *);

const char *sym_to_str(enum sym_type);

#define sym_free(s) free(s)

/* labels */
struct label *symtab_label_find_or_new(symtable *, char *, where *);

/* returns 1 if successful, 0 if said label already exists
 * if 0 is returned, spel is not consumed */
int symtab_label_add_local(symtable *, char *spel/*consumed*/, where *);

void symtab_label_add(symtable *, struct label *);

unsigned sym_hash(const sym *);

#endif
