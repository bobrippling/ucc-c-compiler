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
typedef struct array_decl  array_decl;
typedef struct decl_attr   decl_attr;


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
};

enum type_storage
{
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
	int is_signed;

	/* NULL unless this is a struct, union or enum */
	struct_union_enum_st *sue;

	/* NULL unless from typedef or __typeof() */
	expr *typeof;
};

struct decl_attr
{
	where where;

	enum decl_attr_type
	{
		attr_format,
		attr_unused,
		attr_warn_unused
	} type;

	union
	{
		struct
		{
			enum { attr_fmt_printf, attr_fmt_scanf } fmt_func;
			int fmt_arg, var_arg;
		} format;
	} attr_extra;

	decl_attr *next;
};

/*
 * int *p; // decl -> { desc -> ptr }
 *
 * int  f();   // decl -> { desc -> func }
 * int *f();   // decl -> { desc -> ptr,  child -> func }
 * int (*f)(); // decl -> { desc -> func, child -> ptr }
 *
 * int *(*f)(); // decl -> { desc -> ptr, child -> { func, child -> ptr } }
 *
 * int *(*(*f)())();
 * decl -> {

			desc -> ptr, child -> {
				ptr, child -> {
					func, child -> {
						ptr, child -> {
							func
						}
					}
				}
			}
			???????????????????????????????????????
 */

struct decl_desc
{
	where where;

	enum decl_desc_type
	{
		decl_desc_ptr,
		decl_desc_func,
		decl_desc_array
	} type;

	union
	{
		struct
		{
			int is_const;
			decl_desc *child;
		} ptr;
		struct funcargs
		{
			int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
			decl **arglist;
			int variadic;
		} func;
		struct
		{
			expr *size;      /* int (x[5][2])[2] */
		} array;
	} bits;
};

struct decl
{
	where where;

	int field_width;
	type *type;

	expr *init; /* NULL except for global variables */
	array_decl *arrayinit;

	int ignore; /* ignore during code-gen, for example ignoring overridden externs */
#define struct_offset ignore

	sym *sym;
	decl_attr *attr;
	char *spel;

	int internal; /* interal string or array decl */

	/* recursive */
	decl_desc *desc;

	stmt *func_code;
};

struct array_decl
{
	enum
	{
		array_exprs,
		array_str
	} type;

	union
	{
		expr **exprs;
		char *str;
	} data;

	char *label;
	int len;

	int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
	int args_old_proto; /* true if f(a, b); where a and b are identifiers */
	decl **arglist;
	int variadic;
};


stmt        *tree_new(symtable *stab);
type        *type_new(void);
decl        *decl_new(void);
array_decl  *array_decl_new(void);
decl_attr   *decl_attr_new(enum decl_attr_type);

decl_desc   *decl_desc_new(enum decl_desc_type t);
decl_desc   *decl_desc_array_new(void);

void where_new(struct where *w);

type      *type_copy(type *);
decl      *decl_copy(decl *);
decl_desc *decl_desc_copy(decl_desc *);

const char *op_to_str(  const enum op_type o);
const char *decl_to_str(decl *d);
const char *type_to_str(const type *t);

const char *type_primitive_to_str(const enum type_primitive);
const char *type_qual_to_str(     const enum type_qualifier);
const char *type_store_to_str(    const enum type_storage);

int op_is_cmp(enum op_type o);

enum decl_cmp
{
	DECL_CMP_STRICT_PRIMITIVE = 1 << 0,
	DECL_CMP_ALLOW_VOID_PTR   = 1 << 1,
};

int   type_equal(const type *a, const type *b, int strict);
int   type_size( const type *);
int   decl_size( decl *);
int   decl_equal(decl *, decl *, enum decl_cmp mode);

int   decl_has_array(  decl *);
int   decl_is_struct_or_union(decl *);
int   decl_is_callable(decl *);
int   decl_is_const(   decl *);
int   decl_ptr_depth(  decl *);
int   decl_is_func_ptr(decl *);
#define decl_is_void(d) ((d)->type->primitive == type_void && !(d)->decl_desc)

decl_desc **decl_leaf(decl *d);
decl_desc  *decl_first_func(decl *d);

decl *decl_desc_depth_inc(decl *d);
decl *decl_desc_depth_dec(decl *d);
decl *decl_func_deref(   decl *d);

int decl_attr_present(decl_attr *, enum decl_attr_type);

#define SPEC_STATIC_BUFSIZ 64
#define TYPE_STATIC_BUFSIZ (SPEC_STATIC_BUFSIZ + 64)
#define DECL_STATIC_BUFSIZ (256 + TYPE_STATIC_BUFSIZ)

#define type_free(x) free(x)
#define decl_free_notype(x) do{free(x);}while(0)
#define decl_free(x) do{type_free((x)->type); decl_free_notype(x);}while(0)

#endif
