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
		attr_call_conv,
		attr_nonnull,
		attr_packed,
		attr_sentinel,
		attr_aligned,
		attr_LAST
		/*
		 * TODO: warning
		 * pure - no globals
		 * const - pure + no pointers
		 */
	} type;

	union
	{
		struct
		{
			enum fmt_type
			{
				attr_fmt_printf, attr_fmt_scanf
			} fmt_func;
			int fmt_idx, var_idx, valid;
		} format;
		char *section;
		enum calling_conv
		{
			conv_x64_sysv, /* Linux, FreeBSD and Mac OS X, x64 */
			conv_x64_ms,   /* Windows x64 */
			conv_cdecl,    /* Windows x86 stack, caller cleanup */
			conv_stdcall,  /* Windows x86 stack, callee cleanup */
			conv_fastcall  /* Windows x86, ecx, edx, caller cleanup */
		} conv;
		unsigned long nonnull_args; /* limits to sizeof(long)*8 args, i.e. 64 */
		expr *align, *sentinel;
	} bits;

	decl_attr *next;
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

	expr *field_width;
	unsigned struct_offset;
	unsigned struct_offset_bitfield; /* add onto struct_offset */
	int first_bitfield; /* marker for the first bitfield in a set */

	struct decl_align
	{
		int as_int;
		unsigned resolved;
		union
		{
			expr *align_intk;
			type_ref *align_ty;
		} bits;
		struct decl_align *next;
	} *align;

	int init_normalised;
	int folded;
	int proto_flag;

	/* a reference to a previous prototype, used for attribute checks */
	decl *proto;

	sym *sym;

	decl_init *init; /* initialiser - converted to an assignment for non-globals */
	stmt *func_code;

	/* ^(){} has a decl+sym
	 * the decl/sym has a ref to the expr block,
	 * for pulling off .block.args, etc
	 */
	expr *block_expr;
};

const char *decl_asm_spel(decl *);

#include "type_ref.h"

decl        *decl_new(void);
decl        *decl_new_w(const where *);
decl        *decl_new_ty_sp(type_ref *, char *);
void         decl_replace_with(decl *, decl *);
void         decl_free(decl *, int free_ref);
void         type_ref_free(type_ref *);
void         type_ref_free_1(type_ref *);

void type_ref_init(symtable *stab);
type_ref *type_ref_new_tdef(expr *, decl *);
type_ref *type_ref_new_type(const type *);
type_ref *type_ref_new_type_primitive(enum type_primitive);
type_ref *type_ref_new_type_qual(enum type_primitive, enum type_qualifier);
type_ref *type_ref_new_ptr(  type_ref *to, enum type_qualifier);
type_ref *type_ref_new_block(type_ref *to, enum type_qualifier);
type_ref *type_ref_new_array(type_ref *to, expr *sz);
type_ref *type_ref_new_array2(type_ref *to, expr *sz, enum type_qualifier, int is_static);
type_ref *type_ref_new_func( type_ref *to, funcargs *args);
type_ref *type_ref_new_cast( type_ref *from, enum type_qualifier new);
type_ref *type_ref_new_cast_signed(type_ref *from, int is_signed);
type_ref *type_ref_new_cast_add(type_ref *from, enum type_qualifier extra);
type_ref *type_ref_new_int_enum(struct_union_enum_st *);


decl_attr   *decl_attr_new(enum decl_attr_type);
void         decl_attr_append(decl_attr **loc, decl_attr *new);
const char  *decl_attr_to_str(decl_attr *da);

unsigned decl_size(decl *);
unsigned decl_align(decl *);
unsigned type_ref_size(type_ref *, where *from);
integral_t type_ref_max(type_ref *, where *from);

enum type_cmp decl_cmp(decl *a, decl *b, enum type_cmp_opts opts);
int   decl_store_static_or_extern(enum decl_storage);
int   decl_sort_cmp(const decl **, const decl **);

type_ref *type_ref_ptr_depth_inc(type_ref *);
type_ref *type_ref_ptr_depth_dec(type_ref *r, where *);
type_ref *type_ref_next(type_ref *r);

int decl_conv_array_func_to_ptr(decl *d);
type_ref *decl_is_decayed_array(decl *);
type_ref *type_ref_is_decayed_array(type_ref *);

decl_attr *attr_present(decl_attr *, enum decl_attr_type);
decl_attr *type_attr_present(type_ref *, enum decl_attr_type);
decl_attr *decl_attr_present(decl *, enum decl_attr_type);
decl_attr *expr_attr_present(expr *, enum decl_attr_type);

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[ucc_static_param DECL_STATIC_BUFSIZ], decl *);
const char *type_ref_to_str_r_spel(char buf[ucc_static_param TYPE_REF_STATIC_BUFSIZ], type_ref *r, char *spel);
const char *type_ref_to_str_r(char buf[ucc_static_param TYPE_REF_STATIC_BUFSIZ], type_ref *r);
const char *type_ref_to_str_r_show_decayed(char buf[ucc_static_param TYPE_REF_STATIC_BUFSIZ], type_ref *r);
const char *type_ref_to_str(type_ref *);
const char *decl_store_to_str(const enum decl_storage);

void decl_attr_free(decl_attr *a);

#define DECL_IS_FUNC(d)   type_ref_is((d)->ref, type_ref_func)
#define DECL_IS_ARRAY(d)  type_ref_is((d)->ref, type_ref_array)
#define DECL_IS_S_OR_U(d) type_ref_is_s_or_u((d)->ref)
#define DECL_FUNC_ARG_SYMTAB(d) ((d)->func_code->symtab->parent)

int type_ref_is_variadic_func(type_ref *);

/* type_ref_is_* */
int type_ref_is_complete(type_ref *);
int type_ref_is_variably_modified(type_ref *);
int type_ref_is_void(    type_ref *);
int type_ref_is_integral(type_ref *);
int type_ref_is_bool(    type_ref *);
int type_ref_is_signed(  type_ref *);
int type_ref_is_floating(type_ref *);
int type_ref_is_const(   type_ref *);
int type_ref_is_callable(type_ref *);
int type_ref_is_fptr(    type_ref *);
int type_ref_is_void_ptr(type_ref *);
int type_ref_is_nonvoid_ptr(type_ref *);

type_ref *type_ref_complete_array(type_ref *r, int sz) ucc_wur;
int type_ref_is_incomplete_array(type_ref *);

enum type_qualifier type_ref_qual(const type_ref *);

funcargs *type_ref_funcargs(type_ref *);
enum type_primitive type_ref_primitive(type_ref *);

unsigned type_ref_align(type_ref *, where *from);
unsigned type_ref_array_len(type_ref *);
type_ref *type_ref_is(type_ref *, enum type_ref_type);
type_ref *type_ref_is_type(type_ref *, enum type_primitive);
decl     *type_ref_is_tdef(type_ref *);
type_ref *type_ref_is_ptr(type_ref *); /* returns r->ref iff ptr */
type_ref *type_ref_is_ptr_or_block(type_ref *);
int       type_ref_is_nonfptr(type_ref *);
type_ref *type_ref_is_array(type_ref *); /* returns r->ref iff array */
type_ref *type_ref_func_call(type_ref *, funcargs **pfuncargs);
int       type_ref_decayable(type_ref *r);
type_ref *type_ref_decay(type_ref *);
type_ref *type_ref_is_scalar(type_ref *);
type_ref *type_ref_is_func_or_block(type_ref *);
struct_union_enum_st *type_ref_is_s_or_u(type_ref *);
struct_union_enum_st *type_ref_is_s_or_u_or_e(type_ref *);
struct_union_enum_st *type_ref_is_enum(type_ref *);
type_ref *type_ref_skip_casts(type_ref *);


/* char[] and char *, etc */
enum type_ref_str_type
{
	type_ref_str_no,
	type_ref_str_char,
	type_ref_str_wchar
};
enum type_ref_str_type
type_ref_str_type(type_ref *);

/* note: returns static references */
#define type_ref_cached_VOID()       type_ref_new_type(type_new_primitive(type_void))
#define type_ref_cached_INT()        type_ref_new_type(type_new_primitive(type_int))
#define type_ref_cached_BOOL()       type_ref_new_type(type_new_primitive(type__Bool))
#define type_ref_cached_LONG()       type_ref_new_type(type_new_primitive(type_long))
#define type_ref_cached_LLONG()      type_ref_new_type(type_new_primitive(type_llong))
#define type_ref_cached_DOUBLE()     type_ref_new_type(type_new_primitive(type_double))
#define type_ref_cached_INTPTR_T()   type_ref_cached_LONG()

#define type_ref_cached_CHAR(mode)   type_ref_new_type(type_new_primitive(type_##mode##char))

#define type_ref_cached_VOID_PTR() type_ref_ptr_depth_inc(type_ref_cached_VOID())
#define type_ref_cached_CHAR_PTR(mode) type_ref_ptr_depth_inc(type_ref_cached_CHAR(mode))
#define type_ref_cached_LONG_PTR() type_ref_ptr_depth_inc(type_ref_cached_LONG())
#define type_ref_cached_INT_PTR()  type_ref_ptr_depth_inc(type_ref_cached_INT())

type_ref *type_ref_cached_MAX_FOR(unsigned sz);
type_ref *type_ref_cached_VA_LIST(void);
type_ref *type_ref_cached_VA_LIST_decayed(void);

#endif
