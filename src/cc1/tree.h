#ifndef TREE_H
#define TREE_H

typedef struct expr expr;
typedef struct stmt stmt;
typedef struct stmt_flow stmt_flow;

typedef struct sym         sym;
typedef struct symtable    symtable;
typedef struct symtable_global symtable_global;

typedef struct tdef        tdef;
typedef struct tdeftable   tdeftable;
typedef struct struct_union_enum_st struct_union_enum_st;

typedef struct type        type;
typedef struct decl        decl;
typedef struct funcargs    funcargs;
typedef struct decl_attr   decl_attr;

typedef struct decl_init   decl_init;

enum type_primitive
{
	type_void,
	type__Bool,
	type_char,
#define type_wchar (platform_sys() == PLATFORM_CYGWIN ? type_short : type_int)
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

	type_unknown
};

enum type_qualifier
{
	qual_none     = 0,
	qual_const    = 1 << 0,
	qual_volatile = 1 << 1,
	qual_restrict = 1 << 2,
};

struct type
{
	where where;

	enum type_primitive primitive;
	int is_signed;

	/* NULL unless this is a struct, union or enum */
	struct_union_enum_st *sue;

	/* attr applied to all decls whose type is this type */
	decl_attr *attr;
};

enum type_cmp
{
	TYPE_CMP_EXACT         = 1 << 0,
	TYPE_CMP_ALLOW_SIGNED_UNSIGNED = 1 << 1,
};

const type *type_new_primitive(enum type_primitive);
const type *type_new_primitive_signed(enum type_primitive, int sig);
const type *type_new_primitive_sue(enum type_primitive, struct_union_enum_st *);
#define type_free(x) free(x)

void where_cc1_current(struct where *w);
void where_cc1_adj_identifier(where *w, const char *sp);

const char *op_to_str(  const enum op_type o);
const char *type_to_str(const type *t);

const char *type_primitive_to_str(const enum type_primitive);
const char *type_qual_to_str(     const enum type_qualifier, int trailing_space);

int type_equal(const type *a, const type *b, enum type_cmp mode);
int type_qual_equal(enum type_qualifier, enum type_qualifier);
unsigned type_size( const type *, where *from);
unsigned type_align(const type *, where *from);
unsigned type_primitive_size(enum type_primitive tp);

int op_is_commutative(enum op_type o);
int op_is_relational(enum op_type o);
int op_is_shortcircuit(enum op_type o);
int op_is_comparison(enum op_type o);
int op_can_compound(enum op_type o);


#define SPEC_STATIC_BUFSIZ      64
#define TYPE_STATIC_BUFSIZ      (SPEC_STATIC_BUFSIZ + 64)
#define TYPE_REF_STATIC_BUFSIZ  (TYPE_STATIC_BUFSIZ + 256)
#define DECL_STATIC_BUFSIZ      (TYPE_REF_STATIC_BUFSIZ + 16)


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
