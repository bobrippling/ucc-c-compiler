#ifndef DECL_H
#define DECL_H

#include "type.h"
#include "attribute.h"

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

#define STORE_IS_TYPEDEF(s) (((s) & STORE_MASK_STORE) == store_typedef)

enum decl_init_expr_flags
{
	DECL_INIT_EXPR_FLAGS_BRACE_NORMALIZED,
	DECL_INIT_EXPR_FLAGS_COMPILED_GENERATED,
};

struct decl_init_expr
{
	struct decl_init *dinit;
	struct expr *expr;

	enum decl_init_expr_flags flags;
};
#define DECL_INIT_COMPILER_GENERATED(di) ((di).flags & DECL_INIT_EXPR_FLAGS_COMPILED_GENERATED)

typedef struct decl decl;
struct decl
{
	where where;
	enum decl_storage store;

	struct type *ref; /* should never be null - we always have a ref to a type */

	attribute **attr;

	char *spel, *spel_asm;
	enum {
		DECL_FLAGS_USED = 1 << 0,
		DECL_FLAGS_ADDRESSED = 1 << 1,
		DECL_FLAGS_IMPLICIT = 1 << 2
	} flags;

	union
	{
		struct
		{
			struct expr *field_width;
			type *bitfield_master_ty;
			unsigned struct_offset;
			unsigned struct_offset_bitfield; /* add onto struct_offset */
			int first_bitfield; /* marker for the first bitfield in a set */

			struct
			{
				struct decl_align
				{
					int as_int;
					union
					{
						struct expr *align_intk;
						struct type *align_ty;
					} bits;
					struct decl_align *next;
				} *first;
				unsigned resolved;
			} align;

			/* initialiser - converted to an assignment for non-globals */
			struct decl_init_expr init;
		} var;
		struct
		{
			struct stmt *code;

			/* can't inline static-&& expressions:
			 * static void *x = &&lbl; */
			int contains_static_label_addr;
		} func;
	} bits;

	/* a reference to a previous prototype, used for attribute checks */
	enum
	{
		DECL_FOLD_NO,
		DECL_FOLD_EXCEPT_INIT,
		DECL_FOLD_INIT
	} fold_state;
	int proto_flag;
	struct decl *proto;
	struct decl *impl;

	struct sym *sym;

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

unsigned decl_size(decl *);
unsigned decl_align(decl *);
#define decl_check_size(d) (void)decl_size(d)
void decl_size_align_inc_bitfield(decl *, unsigned *const sz, unsigned *const align);
type *decl_type_for_bitfield(decl *);

enum type_cmp decl_cmp(decl *a, decl *b, enum type_cmp_opts opts);
unsigned decl_hash(const decl *);
int   decl_store_static_or_extern(enum decl_storage);

enum linkage
{
	linkage_none, /* local variables, arguments, typedefs */
	linkage_internal, /* static */
	linkage_external /* extern, global scope */
};
enum linkage decl_linkage(decl *d);
int decl_store_duration_is_static(decl *d); /* i.e. not argument/typedef/local */
int decl_interposable(decl *d);
int decl_needs_GOTPLT(decl *d);

int decl_conv_array_func_to_ptr(decl *d);
struct type *decl_is_decayed_array(decl *);

enum decl_impl_flags
{
	DECL_INCLUDE_ALIAS = 1 << 0,
};

decl *decl_proto(decl *); /* rewinds to the proto */
decl *decl_impl(decl *, enum decl_impl_flags); /* fast-forwards to the impl */
decl *decl_with_init(decl *, enum decl_impl_flags);

int decl_is_pure_inline(decl *);
int decl_should_emit_code(decl *);
int decl_should_emit_var(decl *);
int decl_unused_and_internal(decl *);
enum visibility decl_visibility(decl *);
int decl_defined(decl *, enum decl_impl_flags);

int decl_is_bitfield(decl *);

#define DECL_STATIC_BUFSIZ 512

const char *decl_to_str(decl *d);
const char *decl_to_str_r(char buf[ucc_static_param DECL_STATIC_BUFSIZ], decl *);
const char *decl_store_to_str(const enum decl_storage);

const char *decl_store_spel_type_to_str_r(
		char buf[ucc_static_param DECL_STATIC_BUFSIZ],
		enum decl_storage store,
		const char *spel,
		type *ty);

#define decl_use(d) ((d)->flags |= DECL_FLAGS_USED)

/* compound-literals and block-expressions force
 * their dependants to be generated, even if we don't
 * use them themselves - this macro is used to tag
 * these cases for possible recursive use checks in
 * the future */
#define decl_use_ignoredeps decl_use

#define DECL_FUNC_ARG_SYMTAB(d) ((d)->bits.func.code->symtab->parent)

#define DECL_IS_ANON_BITFIELD(d) \
	((d)->bits.var.field_width && !(d)->spel)

#define DECL_IS_HOSTED_MAIN(fdecl) \
			(!cc1_fopt.freestanding \
			&& !strcmp(fdecl->spel, "main"))

#endif
