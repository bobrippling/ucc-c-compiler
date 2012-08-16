#ifndef DECL_H
#define DECL_H

struct decl_attr
{
	where where;

	enum decl_attr_type
	{
		attr_format,
		attr_unused,
		attr_warn_unused,
		attr_section,
		attr_enum_bitmask,
		attr_noreturn,
		attr_noderef,
		/*
		 * TODO: warning, cdecl, stdcall, fastcall
		 * pure - no globals
		 * const - pure + no pointers
		 */
	} type;

	union
	{
		struct
		{
			enum { attr_fmt_printf, attr_fmt_scanf } fmt_func;
			int fmt_arg, var_arg;
		} format;
		char *section;
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
							func, child -> {
								ptr
							}
						}
					}
				}
			}
 */

struct decl_desc
{
	where where;

	enum decl_desc_type
	{
		decl_desc_ptr,
		decl_desc_block,
		decl_desc_func,
		decl_desc_array,
	} type;

	union
	{
		enum type_qualifier qual;
		struct funcargs
		{
			where where;

			int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
			int args_old_proto; /* true if f(a, b); where a and b are identifiers */
			decl **arglist;
			int variadic;
		} *func;
		expr *array_size;      /* int (x[5][2])[2] */
	} bits;

	decl_desc *child;

	decl_desc *parent_desc;
	decl      *parent_decl;
};

struct decl
{
	where where;

	type *type;
	char *spel;

	int field_width;

	/* no funcargs on the decl - on a desc if it's a decl_desc_func */
	decl_desc *desc;

	decl_init *init; /* initialiser - converted to an assignment for non-globals */
	stmt *func_code;

	int struct_offset;
	sym *sym;
	decl_attr *attr;

	int builtin; /* __builtin_* */

	int is_definition;
	/* true if this is the definition of the decl - may have init or func_code */
	int inline_only;
	/* only inline code - no standalone obj-code generated */
};

struct decl_init
{
	where where;

	enum decl_init_type /* TODO: ops/init_... */
	{
		/*decl_init_str - covered by scalar */
		decl_init_scalar,              /* = [0-9] | basic-expr */
		decl_init_brace,               /* { `decl_init`, `decl_init`, ... } */
		decl_init_struct,              /* { .member1 = `decl_init`, .member2 = `decl_init` } */
	} type;

	union
	{
		expr *expr;

		struct decl_init_sub
		{
			where where;
			decl_init *init;
			/* only for struct inits */
			char *spel;
		} **subs;
	} bits;

	decl *for_decl;
	/* the decl this is for - may be a struct, struct member, or:
	 * int x[] = { 1, 2, 3 }; - a copy of x's decl, dereferenced
	 */
};

struct data_store
{
	enum
	{
		data_store_str
	} type;

	union
	{
		char *str;
	} bits;
	int len;

	char *spel; /* asm */
};

enum decl_cmp
{
	DECL_CMP_STRICT_PRIMITIVE = 1 << 0,
	DECL_CMP_ALLOW_VOID_PTR   = 1 << 1,
};

decl        *decl_new(void);
decl        *decl_new_type(enum type_primitive p);
#define      decl_new_void() decl_new_type(type_void)
array_decl  *array_decl_new(void);
decl_attr   *decl_attr_new(enum decl_attr_type);
void         decl_attr_append(decl_attr **loc, decl_attr *new);
const char  *decl_attr_to_str(enum decl_attr_type);

void         decl_desc_append(decl_desc **parent, decl_desc *child);
decl_desc   *decl_desc_tail(decl *);

decl_desc   *decl_desc_new(enum decl_desc_type t, decl *dparent, decl_desc *parent);
decl_desc   *decl_desc_ptr_new(  decl *dparent, decl_desc *parent);
decl_desc   *decl_desc_block_new(decl *dparent, decl_desc *parent);
decl_desc   *decl_desc_func_new( decl *dparent, decl_desc *parent);
decl_desc   *decl_desc_array_new(decl *dparent, decl_desc *parent);

#define decl_desc_ptr_or_block(d) ((d)->type == decl_desc_ptr || (d)->type == decl_desc_block)

decl      *decl_copy(decl *);
decl_desc *decl_desc_copy(decl_desc *);

decl_init *decl_init_new(enum decl_init_type);
decl_init_sub *decl_init_sub_new(void);
decl_init_sub *decl_init_sub_zero_for_decl(decl *);
int        decl_init_len(decl_init *);
const char *decl_init_to_str(enum decl_init_type);
#define decl_init_is_brace(di) ((di)->type == decl_init_brace || (di)->type == decl_init_struct)

void decl_conv_array_func_to_ptr(decl *d);

void decl_desc_link(decl *);

int   decl_size( decl *);
int   decl_equal(decl *, decl *, enum decl_cmp mode);

int     decl_is_struct_or_union(decl *);
int     decl_is_struct_or_union_possible_ptr(decl *);
int     decl_is_callable(       decl *);
int     decl_is_func(           decl *); /* different from _callable - fptrs are also callable */
int     decl_is_const(          decl *);
int     decl_is_fptr(           decl *);

int     decl_is_void_ptr(       decl *);
int     decl_ptr_depth(         decl *);
int     decl_desc_depth(        decl *);
int     decl_is_integral(       decl *);
#define decl_non_ptr_type(d, t) (!(d)->desc && (d)->type->primitive == t)
#define decl_is_void(d) decl_non_ptr_type(d, type_void)
#define decl_is_bool(d) (decl_ptr_depth(d) || decl_is_integral(d))
#define decl_is_definition(d) ((d)->init || (d)->func_code)

decl_desc  *decl_first_func(decl *d);
decl_desc  *decl_leaf(decl *d);

decl *decl_ptr_depth_inc(decl *d);
decl *decl_ptr_depth_dec(decl *d, where *from);
decl *decl_func_deref(decl *d, funcargs **pfuncargs);

decl_desc *decl_array_incomplete(decl *d);
decl_desc *decl_array_first(decl *d);

int decl_attr_present(decl_attr *, enum decl_attr_type);

int decl_has_array(decl *);
int decl_has_incomplete_array(decl *);
int decl_is_array( decl *);
funcargs *decl_funcargs(decl *);
int funcargs_equal(funcargs *args_a, funcargs *args_b, int strict_types, int *idx);

const char *decl_desc_to_str(enum decl_desc_type t);

void  decl_set_spel(decl *, char *);

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *);

void decl_desc_free(decl_desc *);
#define decl_free_notype(x) do{free(x);}while(0)
void decl_free(decl *d);

#endif
