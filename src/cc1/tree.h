#ifndef TREE_H
#define TREE_H

typedef struct expr expr;
typedef struct stmt stmt;
typedef struct stmt_flow stmt_flow;

typedef struct sym         sym;
typedef struct symtable    symtable;

typedef struct tdef        tdef;
typedef struct tdeftable   tdeftable;
typedef struct struct_union_enum_st struct_union_enum_st;

typedef struct type        type;
typedef struct decl        decl;
typedef struct decl_desc   decl_desc;
typedef struct funcargs    funcargs;
typedef struct decl_attr   decl_attr;

typedef struct decl_init   decl_init;
typedef struct decl_init_sub decl_init_sub;
typedef struct data_store  data_store;

enum type_primitive
{
	type_int,
	type_char,
	type_void,

	type_struct,
	type_union,
	type_enum,

	type_unknown
};

enum type_qualifier
{
	qual_none     = 0,
	qual_const    = 1 << 0,
	qual_volatile = 1 << 1,
	qual_restrict = 1 << 2,
};

enum type_storage
{
	store_default, /* auto or external-linkage depending on scope */
	store_auto,
	store_static,
	store_extern,
	store_register,
	store_typedef
};

#define type_store_static_or_extern(x) ((x) == store_static || (x) == store_extern)

struct type
{
	where where;

	enum type_primitive primitive;
	enum type_qualifier qual;
	enum type_storage   store;
	int is_signed, is_inline;

	/* NULL unless this is a struct, union or enum */
	struct_union_enum_st *sue;

	/* NULL unless from typedef or __typeof() */
	expr *typeof;

	/* attr applied to all decls whose type is this type */
	decl_attr *attr;
};

type *type_new(void);
type *type_copy(type *);

void where_new(struct where *w);

const char *op_to_str(  const enum op_type o);
const char *type_to_str(const type *t);

const char *type_primitive_to_str(const enum type_primitive);
const char *type_qual_to_str(     const enum type_qualifier);
const char *type_store_to_str(    const enum type_storage);

int op_is_cmp(enum op_type o);

int   type_equal(const type *a, const type *b, int strict);
int   type_size( const type *);
funcargs *funcargs_new(void);
void function_empty_args(funcargs *func);

void funcargs_free(funcargs *args, int free_decls);

#define SPEC_STATIC_BUFSIZ 64
#define TYPE_STATIC_BUFSIZ (SPEC_STATIC_BUFSIZ + 64)
#define DECL_STATIC_BUFSIZ (256 + TYPE_STATIC_BUFSIZ)

#define type_free(x) free(x)

/* tables local to the current scope */
extern symtable *current_scope;

#endif
