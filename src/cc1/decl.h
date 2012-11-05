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

struct type_ref
{
	where where;
	type_ref *ref;

	decl_attr *attr;

	enum type_ref_type
	{
		type_ref_type,  /* end - at type */
		type_ref_tdef,  /* type reference to next ref */
		type_ref_ptr,   /* pointer to next ref */
		type_ref_block, /* block pointer to next ref (func) */
		type_ref_func,  /* function */
		type_ref_array, /* array of next ref, similar to pointer */
		type_ref_cast   /* used for adding qualifiers */
	} type;

	union
	{
		/* ref_type */
		type *type;

		/* ref_tdef */
		expr *type_of; /* typedef if NULL else typeof */

		/* ref_ptr, ref_cast */
		enum type_qualifier qual;

		struct funcargs /* ref_func */
		{
			where where;

			int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
			int args_old_proto; /* true if f(a, b); where a and b are identifiers */
			decl **arglist;
			int variadic;
		} *func;

		/* ref_block */
		struct
		{
			struct funcargs *func;
			enum type_qualifier qual;
		} block;

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

	type_ref *ref; /* should never be null - we always have a ref to a type */

	decl_attr *attr;

	/* things we want immediately */
	char *spel;

#ifdef FIELD_WIDTH_TODO
	expr *field_width;
#endif
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
void         decl_free(decl *), type_ref_free(type_ref *);

type_ref *type_ref_new_tdef(expr *);
type_ref *type_ref_new_type(type *);
type_ref *type_ref_new_ptr(  type_ref *to, enum type_qualifier);
type_ref *type_ref_new_block(type_ref *to, enum type_qualifier);
type_ref *type_ref_new_array(type_ref *to, expr *sz);
type_ref *type_ref_new_func( type_ref *to, funcargs *args);
type_ref *type_ref_new_cast( type_ref *from, enum type_qualifier extra);


decl_attr   *decl_attr_new(enum decl_attr_type);
void         decl_attr_append(decl_attr **loc, decl_attr *new);
const char  *decl_attr_to_str(enum decl_attr_type);

int   decl_size( decl *);
int   type_ref_size(type_ref *);
int   decl_equal(decl *a, decl *b, enum decl_cmp mode);
int   type_ref_equal(type_ref *a, type_ref *b, enum decl_cmp mode);

decl *decl_ptr_depth_inc(decl *);
decl *decl_ptr_depth_dec(decl *, where *from);

type_ref *type_ref_ptr_depth_inc(type_ref *);
type_ref *type_ref_ptr_depth_dec(type_ref *, where *from);
type_ref *type_ref_decay_first_array(type_ref *);

type *decl_get_type(decl *);

int   decl_ptr_depth(    decl *d);
decl *decl_decay_first_array(decl *);
void decl_complete_array(decl *, int to);
void decl_conv_array_func_to_ptr(decl *d);

int decl_attr_present(decl_attr *, enum decl_attr_type);

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *);
const char *type_ref_to_str_r(char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *);
const char *type_ref_to_str(const type_ref *);
const char *decl_store_to_str(const enum decl_storage);

void decl_attr_free(decl_attr *a);

/* decl_is_* */
#define decl_is_definition(d) ((d)->init || (d)->func_code)

type_ref *decl_is(decl *d, enum type_ref_type t);
int decl_is_func(decl *d);
int decl_is_ptr(decl *d);
int decl_is_array(decl *d);
int decl_is_incomplete_array(decl *d);
int decl_is_variadic(decl *d);
int decl_is_fptr(decl *d);
struct_union_enum_st *decl_is_s_or_u(decl *d);

/* type_ref_is_* */
int type_ref_is_complete(   const type_ref *r);
int type_ref_is_void(       const type_ref *r);
int type_ref_is_integral(   const type_ref *);
int type_ref_is_bool(       const type_ref *);
int type_ref_is_signed(     const type_ref *);
int type_ref_is_floating(   const type_ref *);
int type_ref_is_const(      const type_ref *);
int type_ref_is_callable(   const type_ref *);

enum type_qualifier type_ref_qual(const type_ref *);

funcargs *type_ref_funcargs(const type_ref *);

type_ref *type_ref_is(type_ref *, enum type_ref_type, ...);
type_ref *type_ref_func_call(type_ref *, funcargs **pfuncargs);
struct_union_enum_st *type_ref_is_s_or_u(type_ref *);

#define decl_is_void(d) decl_is_type(d, type_void)
#define decl_is_bool(d) (decl_is_ptr(d) || decl_is_integral(d))

#define type_ref_new_VOID() type_ref_new_type(type_new_primitive(type_void))
#define type_ref_new_INT()  type_ref_new_type(type_new_primitive(type_int))
#define type_ref_new_CHAR() type_ref_new_type(type_new_primitive(type_char))
#define type_ref_new_BOOL() type_ref_new_type(type_new_primitive(type_int))

#endif
