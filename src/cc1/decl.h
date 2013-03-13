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
		attr_nonnull,
		attr_packed,
		attr_sentinel,
#define attr_LAST (attr_sentinel + 1)
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
			unsigned fmt_arg, var_arg;
		} format;
		char *section;
		unsigned long nonnull_args; /* limits to sizeof(long)*8 args, i.e. 64 */
		unsigned sentinel;
	} attr_extra;

	decl_attr *next;
};

struct type_ref
{
	where where;
	type_ref *ref, *tmp; /* tmp used for things like printing */

	decl_attr *attr;
	int folded;

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
		struct type_ref_tdef
		{
			expr *type_of;
			decl *decl;
		} tdef;

		/* ref_{ptr,array} */
		struct
		{
			enum type_qualifier qual;
			int is_static;
			expr *size;
			/* when we decay
			 * f(int x[2]) -> f(int *x)
			 * we save the size + is_static
			 */
		} ptr, array;

		/* ref_cast */
		struct
		{
			char is_signed_cast; /* if true - signed_true else qual */
			char signed_true;
			char additive; /* replace qual or add? */
			enum type_qualifier qual;
		} cast;

		/* ref_func */
		struct funcargs
		{
			where where;

			int args_void_implicit; /* f(){} - implicitly (void) */
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
  store_inline = 1 << 3
};
#define STORE_MASK_STORE 0x00007 /* include all below 4 */
#define STORE_MASK_EXTRA 0xfff38 /* exclude  ^ */

struct decl
{
	where where;
	enum decl_storage store;

	type_ref *ref; /* should never be null - we always have a ref to a type */

	decl_attr *attr;

	char *spel, *spel_asm; /* if !spel but spel_asm, it's global asm??? */

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

#define decl_asm_spel(d) ((d)->spel_asm ? (d)->spel_asm : (d)->spel)

enum decl_cmp
{
	DECL_CMP_EXACT_MATCH    = 1 << 0,
	DECL_CMP_ALLOW_VOID_PTR = 1 << 1,
	DECL_CMP_ALLOW_SIGNED_UNSIGNED = 1 << 2,
};

decl        *decl_new(void);
decl        *decl_new_type(enum type_primitive p);
#define      decl_new_void() decl_new_type(type_void)
#define      decl_new_char() decl_new_type(type_char)
#define      decl_new_int()  decl_new_type(type_int)
void         decl_free(decl *, int free_ref);
void         type_ref_free(type_ref *);
void         type_ref_free_1(type_ref *);

type_ref *type_ref_new_tdef(expr *, decl *);
type_ref *type_ref_new_type(type *);
type_ref *type_ref_new_ptr(  type_ref *to, enum type_qualifier);
type_ref *type_ref_new_block(type_ref *to, enum type_qualifier);
type_ref *type_ref_new_array(type_ref *to, expr *sz);
type_ref *type_ref_new_array2(type_ref *to, expr *sz, enum type_qualifier, int is_static);
type_ref *type_ref_new_func( type_ref *to, funcargs *args);
type_ref *type_ref_new_cast( type_ref *from, enum type_qualifier new);
type_ref *type_ref_new_cast_signed(type_ref *from, int is_signed);
type_ref *type_ref_new_cast_add(type_ref *from, enum type_qualifier extra);


decl_attr   *decl_attr_new(enum decl_attr_type);
void         decl_attr_append(decl_attr **loc, decl_attr *new);
const char  *decl_attr_to_str(enum decl_attr_type);

unsigned decl_size(decl *, where const *from);
unsigned type_ref_size(type_ref *, where const *from);

int   decl_equal(decl *a, decl *b, enum decl_cmp mode);
int   type_ref_equal(type_ref *a, type_ref *b, enum decl_cmp mode);
int   decl_store_static_or_extern(enum decl_storage);

decl *decl_ptr_depth_inc(decl *);
decl *decl_ptr_depth_dec(decl *);

type_ref *type_ref_ptr_depth_inc(type_ref *);
type_ref *type_ref_ptr_depth_dec(type_ref *);
type_ref *type_ref_next(type_ref *r);

type *type_ref_get_type(type_ref *);
type *decl_get_type(decl *);

int decl_conv_array_func_to_ptr(decl *d);
type_ref *decl_is_decayed_array(decl *);
type_ref *type_ref_is_decayed_array(type_ref *);

decl_attr *decl_attr_present(decl_attr *, enum decl_attr_type);
decl_attr *type_attr_present(type_ref *, enum decl_attr_type);
decl_attr *decl_has_attr(decl *, enum decl_attr_type);

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *);
const char *type_ref_to_str_r_spel(char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *r, char *spel);
const char *type_ref_to_str_r(char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *r);
const char *type_ref_to_str(type_ref *);
const char *decl_store_to_str(const enum decl_storage);

void decl_attr_free(decl_attr *a);

/* decl_is_* */
#define decl_is_definition(d) ((d)->init || (d)->func_code)

#define DECL_IS_FUNC(d)   type_ref_is((d)->ref, type_ref_func)
#define DECL_IS_ARRAY(d)  type_ref_is((d)->ref, type_ref_array)
#define DECL_IS_S_OR_U(d) type_ref_is_s_or_u((d)->ref)

int decl_is_variadic(decl *d);

/* type_ref_is_* */
int type_ref_is_complete(type_ref *);
int type_ref_is_void(    type_ref *);
int type_ref_is_integral(type_ref *);
int type_ref_is_bool(    type_ref *);
int type_ref_is_signed(  type_ref *);
int type_ref_is_floating(type_ref *);
int type_ref_is_const(   type_ref *);
int type_ref_is_callable(type_ref *);
int type_ref_is_fptr(    type_ref *);

type_ref *type_ref_complete_array(type_ref *r, int sz) ucc_wur;
int type_ref_is_incomplete_array(type_ref *);

enum type_qualifier type_ref_qual(const type_ref *);

funcargs *type_ref_funcargs(type_ref *);

int type_ref_align(type_ref *, where const *from);
long type_ref_array_len(type_ref *);
type_ref *type_ref_is(type_ref *, enum type_ref_type);
type_ref *type_ref_is_type(type_ref *, enum type_primitive);
decl     *type_ref_is_tdef(type_ref *);
type_ref *type_ref_is_ptr(type_ref *); /* returns r->ref iff ptr */
type_ref *type_ref_func_call(type_ref *, funcargs **pfuncargs);
type_ref *type_ref_decay(type_ref *);
struct_union_enum_st *type_ref_is_s_or_u(type_ref *);
struct_union_enum_st *type_ref_is_s_or_u_or_e(type_ref *);

#define decl_is_void(d) decl_is_type(d, type_void)
#define decl_is_bool(d) (decl_is_ptr(d) || decl_is_integral(d))

#define type_ref_cached_VOID()       type_ref_new_type(type_new_primitive(type_void))
#define type_ref_cached_INT()        type_ref_new_type(type_new_primitive(type_int))
#define type_ref_cached_CHAR()       type_ref_new_type(type_new_primitive(type_char))
#define type_ref_cached_BOOL()       type_ref_new_type(type_new_primitive(type_int))
#define type_ref_cached_INTPTR_T()   type_ref_new_type(type_new_primitive(type_long))

#define type_ref_cached_VOID_PTR() type_ref_ptr_depth_inc(type_ref_cached_VOID())
#define type_ref_cached_CHAR_PTR() type_ref_ptr_depth_inc(type_ref_cached_CHAR())

type_ref *type_ref_cached_MAX_FOR(unsigned sz);


#endif
