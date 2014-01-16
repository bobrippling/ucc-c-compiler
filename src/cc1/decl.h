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
			conv_cdecl,    /* All 32-bit x86 systems, stack, caller cleanup */
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

	type *ref; /* should never be null - we always have a ref to a type */

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
			type *align_ty;
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
	int func_var_offset;

	/* ^(){} has a decl+sym
	 * the decl/sym has a ref to the expr block,
	 * for pulling off .block.args, etc
	 */
	expr *block_expr;
};

const char *decl_asm_spel(decl *);

#include "type.h"

decl        *decl_new(void);
decl        *decl_new_w(const where *);
decl        *decl_new_ty_sp(type *, char *);
void         decl_replace_with(decl *, decl *);
void         decl_free(decl *, int free_ref);
void         type_free(type *);
void         type_free_1(type *);

void type_init(symtable *stab);
type *type_new_tdef(expr *, decl *);
type *type_new_type(const btype *);
type *type_new_type_primitive(enum type_primitive);
type *type_new_type_qual(enum type_primitive, enum type_qualifier);
type *type_new_ptr(  type *to, enum type_qualifier);
type *type_new_block(type *to, enum type_qualifier);
type *type_new_array(type *to, expr *sz);
type *type_new_array2(type *to, expr *sz, enum type_qualifier, int is_static);
type *type_new_func(type *, funcargs *, symtable *arg_scope);
type *type_new_cast( type *from, enum type_qualifier new);
type *type_new_cast_signed(type *from, int is_signed);
type *type_new_cast_add(type *from, enum type_qualifier extra);


decl_attr   *decl_attr_new(enum decl_attr_type);
void         decl_attr_append(decl_attr **loc, decl_attr *new);
const char  *decl_attr_to_str(decl_attr *da);

unsigned decl_size(decl *);
unsigned decl_align(decl *);
unsigned type_size(type *, where *from);
integral_t type_max(type *, where *from);

enum type_cmp decl_cmp(decl *a, decl *b, enum type_cmp_opts opts);
int   decl_store_static_or_extern(enum decl_storage);

type *type_ptr_depth_inc(type *);
type *type_ptr_depth_dec(type *r, where *);
type *type_next(type *r);

int decl_conv_array_func_to_ptr(decl *d);
type *decl_is_decayed_array(decl *);
type *type_is_decayed_array(type *);

decl_attr *attr_present(decl_attr *, enum decl_attr_type);
decl_attr *type_attr_present(type *, enum decl_attr_type);
decl_attr *decl_attr_present(decl *, enum decl_attr_type);
decl_attr *expr_attr_present(expr *, enum decl_attr_type);

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[ucc_static_param DECL_STATIC_BUFSIZ], decl *);
const char *type_to_str_r_spel(char buf[ucc_static_param TYPE_REF_STATIC_BUFSIZ], type *r, char *spel);
const char *type_to_str_r(char buf[ucc_static_param TYPE_REF_STATIC_BUFSIZ], type *r);
const char *type_to_str_r_show_decayed(char buf[ucc_static_param TYPE_REF_STATIC_BUFSIZ], type *r);
const char *type_to_str(type *);
const char *decl_store_to_str(const enum decl_storage);

void decl_attr_free(decl_attr *a);

#define DECL_IS_FUNC(d)   type_is((d)->ref, type_func)
#define DECL_IS_ARRAY(d)  type_is((d)->ref, type_array)
#define DECL_IS_S_OR_U(d) type_is_s_or_u((d)->ref)
#define DECL_FUNC_ARG_SYMTAB(d) ((d)->func_code->symtab->parent)

int type_is_variadic_func(type *);

/* type_is_* */
int type_is_complete(type *);
int type_is_variably_modified(type *);
int type_is_void(    type *);
int type_is_integral(type *);
int type_is_bool(    type *);
int type_is_signed(  type *);
int type_is_floating(type *);
int type_is_const(   type *);
int type_is_callable(type *);
int type_is_fptr(    type *);
int type_is_void_ptr(type *);
int type_is_nonvoid_ptr(type *);

type *type_complete_array(type *r, int sz) ucc_wur;
int type_is_incomplete_array(type *);

enum type_qualifier type_qual(const type *);

funcargs *type_funcargs(type *);
enum type_primitive type_primitive(type *);

unsigned type_align(type *, where *from);
unsigned type_array_len(type *);
type *type_is(type *, enum type_type);
type *type_is_primitive(type *, enum type_primitive);
decl     *type_is_tdef(type *);
type *type_is_ptr(type *); /* returns r->ref iff ptr */
type *type_is_ptr_or_block(type *);
int       type_is_nonfptr(type *);
type *type_is_array(type *); /* returns r->ref iff array */
type *type_func_call(type *, funcargs **pfuncargs);
int       type_decayable(type *r);
type *type_decay(type *);
type *type_is_scalar(type *);
type *type_is_func_or_block(type *);
struct_union_enum_st *type_is_s_or_u(type *);
struct_union_enum_st *type_is_s_or_u_or_e(type *);
type *type_skip_casts(type *);


/* char[] and char *, etc */
enum type_str_type
{
	type_str_no,
	type_str_char,
	type_str_wchar
};
enum type_str_type
type_str_type(type *);

/* note: returns static references */
#define type_cached_VOID()       type_new_type(type_new_primitive(type_void))
#define type_cached_INT()        type_new_type(type_new_primitive(type_int))
#define type_cached_BOOL()       type_new_type(type_new_primitive(type__Bool))
#define type_cached_LONG()       type_new_type(type_new_primitive(type_long))
#define type_cached_LLONG()      type_new_type(type_new_primitive(type_llong))
#define type_cached_ULONG()      type_new_type(type_new_primitive_signed(type_long, 0))
#define type_cached_DOUBLE()     type_new_type(type_new_primitive(type_double))
#define type_cached_INTPTR_T()   type_cached_LONG()

#define type_cached_CHAR(mode)   type_new_type(type_new_primitive(type_##mode##char))

#define type_cached_VOID_PTR() type_ptr_depth_inc(type_cached_VOID())
#define type_cached_CHAR_PTR(mode) type_ptr_depth_inc(type_cached_CHAR(mode))
#define type_cached_LONG_PTR() type_ptr_depth_inc(type_cached_LONG())
#define type_cached_INT_PTR()  type_ptr_depth_inc(type_cached_INT())

type *type_cached_MAX_FOR(unsigned sz);
type *type_cached_VA_LIST(void);
type *type_cached_VA_LIST_decayed(void);

#endif
