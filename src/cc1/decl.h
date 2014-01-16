#ifndef DECL_H
#define DECL_H

#include "type.h"

typedef struct decl_attr decl_attr;
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
		struct expr *align, *sentinel;
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

typedef struct decl decl;
struct decl
{
	where where;
	enum decl_storage store;

	struct type *ref; /* should never be null - we always have a ref to a type */

	decl_attr *attr;

	char *spel, *spel_asm; /* if !spel but spel_asm, it's global asm??? */

	struct expr *field_width;
	unsigned struct_offset;
	unsigned struct_offset_bitfield; /* add onto struct_offset */
	int first_bitfield; /* marker for the first bitfield in a set */

	struct decl_align
	{
		int as_int;
		unsigned resolved;
		union
		{
			struct expr *align_intk;
			struct type *align_ty;
		} bits;
		struct decl_align *next;
	} *align;

	int init_normalised;
	int folded;
	int proto_flag;

	/* a reference to a previous prototype, used for attribute checks */
	struct decl *proto;

	struct sym *sym;

	struct decl_init *init; /* initialiser - converted to an assignment for non-globals */
	struct stmt *func_code;
	int func_var_offset;

	/* ^(){} has a decl+sym
	 * the decl/sym has a ref to the expr block,
	 * for pulling off .block.args, etc
	 */
	struct expr *block_expr;
};

const char *decl_asm_spel(decl *);

decl        *decl_new(void);
decl        *decl_new_w(const where *);
decl        *decl_new_ty_sp(struct type *, char *);
void         decl_replace_with(decl *, decl *);
void         decl_free(decl *);

decl_attr   *decl_attr_new(enum decl_attr_type);
void         decl_attr_append(decl_attr **loc, decl_attr *new);
const char  *decl_attr_to_str(decl_attr *da);

unsigned decl_size(decl *);
unsigned decl_align(decl *);

enum type_cmp decl_cmp(decl *a, decl *b, enum type_cmp_opts opts);
int   decl_store_static_or_extern(enum decl_storage);

int decl_conv_array_func_to_ptr(decl *d);
struct type *decl_is_decayed_array(decl *);

decl_attr *attr_present(decl_attr *, enum decl_attr_type);
decl_attr *type_attr_present(struct type *, enum decl_attr_type);
decl_attr *decl_attr_present(decl *, enum decl_attr_type);
decl_attr *expr_attr_present(struct expr *, enum decl_attr_type);

#define DECL_STATIC_BUFSIZ 512

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[ucc_static_param DECL_STATIC_BUFSIZ], decl *);
const char *decl_store_to_str(const enum decl_storage);

void decl_attr_free(struct decl_attr *a);

#define DECL_IS_FUNC(d)   type_is((d)->ref, type_func)
#define DECL_IS_ARRAY(d)  type_is((d)->ref, type_array)
#define DECL_IS_S_OR_U(d) type_is_s_or_u((d)->ref)
#define DECL_FUNC_ARG_SYMTAB(d) ((d)->func_code->symtab->parent)

#endif
