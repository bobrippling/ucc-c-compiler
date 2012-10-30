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

struct decl_ref
{
	where where;
	decl_ref *ref;

	decl_attr *attr;

	enum decl_ref_type
	{
		decl_ref_type,  /* end - at type */
		decl_ref_tdef,  /* type reference to next ref */
		decl_ref_ptr,   /* pointer to next ref */
		decl_ref_block, /* block pointer to next ref (func) */
		decl_ref_func,  /* function */
		decl_ref_array  /* array of next ref, similar to pointer */
	} type;

	union
	{
		/* ref_type */
		type *type;

		/* ref_tdef */
		expr *type_of;

		/* ref_ptr */
		enum type_qualifier qual;

		struct funcargs /* ref_{func,block} */
		{
			where where;

			int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
			int args_old_proto; /* true if f(a, b); where a and b are identifiers */
			decl **arglist;
			int variadic;
		} *func;

		/* ref_array */
		expr *array_size;
	} bits;
};

enum decl_storage
{
  /* auto or external-linkage depending on scope + other defs */
  store_default   = 0,
  store_auto      ,
  store_static    ,
  store_extern    ,
  store_register  ,
  store_typedef   , /* 5 - next power of two is 8 */
  store_inline = 1 << 4
};
#define STORE_MASK_STORE 0x00003 /* include all below 4 */
#define STORE_MASK_EXTRA 0xffff4 /* exclude  ^ */

#define decl_store_static_or_extern(x) ((x) == store_static || (x) == store_extern)

struct decl
{
	where where;
	enum decl_storage store;
	int is_inline;

	decl_ref *ref; /* should never be null - we always have a ref to a type */

	decl_attr *attr;

	/* things we want immediately */
	char *spel;

	expr *field_width;
	int struct_offset;

	sym *sym;

	decl_init *init; /* initialiser - converted to an assignment for non-globals */
	stmt *func_code;

	int is_definition;
	/* true if this is the definition of the decl - may have init or func_code */
	int inline_only;
	/* only inline code - no standalone obj-code generated */
};

enum decl_cmp
{
	DECL_CMP_EXACT_MATCH    = 1 << 0,
	DECL_CMP_ALLOW_VOID_PTR = 1 << 1,
};

decl        *decl_new(void);
decl        *decl_new_type(enum type_primitive p);
#define      decl_new_void() decl_new_type(type_void)
#define      decl_new_char() decl_new_type(type_char)
#define      decl_new_int()  decl_new_type(type_int)
void         decl_free(decl *), decl_ref_free(decl_ref *);

decl_ref *decl_ref_new_tdef(expr *);
decl_ref *decl_ref_new_type(type *);
decl_ref *decl_ref_new_ptr(  decl_ref *to, enum type_qualifier);
decl_ref *decl_ref_new_block(decl_ref *to, enum type_qualifier);
decl_ref *decl_ref_new_array(decl_ref *to, expr *sz);
decl_ref *decl_ref_new_func( decl_ref *to, funcargs *args);


decl_attr   *decl_attr_new(enum decl_attr_type);
void         decl_attr_append(decl_attr **loc, decl_attr *new);
const char  *decl_attr_to_str(enum decl_attr_type);

int   decl_size( decl *);
int   decl_ref_size(decl_ref *);
int   decl_equal(decl *, decl *, enum decl_cmp mode);

decl *decl_ptr_depth_inc(decl *);
decl *decl_ptr_depth_dec(decl *, where *from);

decl_ref *decl_ref_ptr_depth_inc(decl_ref *);
decl_ref *decl_ref_ptr_depth_dec(decl_ref *, where *from);

type *decl_get_type(decl *);

decl *decl_func_deref(decl *d, funcargs **pfuncargs);
int   decl_ptr_depth(    decl *d);
decl *decl_decay_first_array(decl *);
void decl_complete_array(decl *, int to);
void decl_conv_array_func_to_ptr(decl *d);

int decl_attr_present(decl_attr *, enum decl_attr_type);

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *);
const char *decl_ref_to_str(enum decl_ref_type t);
const char *decl_store_to_str(const enum decl_storage);

void decl_attr_free(decl_attr *a);

/* decl_is_* */
#define decl_is_definition(d) ((d)->init || (d)->func_code)

decl_ref *decl_is(decl *d, enum decl_ref_type t);
int decl_is_void_ptr(decl *d);
int decl_is_ptr(decl *d);
int decl_is_integral(decl *d);
int decl_is_func(decl *d);
int decl_is_block(decl *d);
int decl_is_struct(decl *d);
int decl_is_type(decl *d, enum type_primitive t);
int decl_is_signed(decl *d);
int decl_is_array(decl *d);
int decl_is_incomplete_array(decl *d);
struct_union_enum_st *decl_is_s_or_u(decl *d);

/* decl_ref_is_* */
int decl_ref_is_complete(decl_ref *r);

#define decl_is_void(d) decl_is_type(d, type_void)
#define decl_is_bool(d) (decl_is_ptr(d) || decl_is_integral(d))

#endif
