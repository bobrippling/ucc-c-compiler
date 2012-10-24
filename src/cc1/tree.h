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
typedef struct decl_ref    decl_ref;
typedef struct funcargs    funcargs;
typedef struct decl_attr   decl_attr;

typedef struct decl_init   decl_init;
typedef struct data_store  data_store;

enum type_primitive
{
	type_void,
	type__Bool,
	type_char,
	type_int,
	type_short,
	type_long,
	type_llong,
	type_float,
	type_double,
	type_ldouble,

	type_struct,
	type_union,
	type_enum,

	/* implicitly unsigned */
	type_intptr_t,
	type_ptrdiff_t,

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
	store_default, /* auto or external-linkage depending on scope + other defs */
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

	/* attr applied to all decls whose type is this type */
	decl_attr *attr;
};

enum type_cmp
{
	TYPE_CMP_EXACT         = 1 << 0,
	TYPE_CMP_QUAL          = 1 << 1,
};


type *type_new(void);
type *type_copy(type *);
#define type_free(x) free(x)

void where_new(struct where *w);

const char *op_to_str(  const enum op_type o);
const char *type_to_str(const type *t);

const char *type_primitive_to_str(const enum type_primitive);
const char *type_store_to_str(    const enum type_storage);
      char *type_qual_to_str(     const enum type_qualifier);

int type_equal(const type *a, const type *b, enum type_cmp mode);
int type_size( const type *);
int type_primitive_size(enum type_primitive tp);

int op_is_relational(enum op_type o);
int op_can_compound(enum op_type o);


#define SPEC_STATIC_BUFSIZ 64
#define TYPE_STATIC_BUFSIZ (SPEC_STATIC_BUFSIZ + 64)
#define DECL_STATIC_BUFSIZ (256 + TYPE_STATIC_BUFSIZ)


/* tables local to the current scope */
extern symtable *current_scope;
intval *intval_new(long v);

extern const where *eof_where;

#define EOF_WHERE(exp, code)                 \
	do{                                        \
		const where *const eof_save = eof_where; \
		eof_where = (exp);                       \
		{ code; }                                \
		eof_where = eof_save;                    \
	}while(0)

#endif
