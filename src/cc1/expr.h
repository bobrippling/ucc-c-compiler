#ifndef EXPR_H
#define EXPR_H

#define ASM_INLINE_FNAME "__asm__"

#include "strings.h"
#include "type.h"
#include "op.h"
#include "sym.h"
#include "decl.h"
#include "const.h"
#include "out/out.h"
#include "../util/compiler.h"

enum lvalue_kind
{
	LVALUE_NO,
	LVALUE_USER_ASSIGNABLE,
	LVALUE_STRUCT /* e.g. struct A f(); f() - is an lvalue internally */
};

typedef void func_fold(struct expr *, struct symtable *);
typedef void func_const(struct expr *, consty *);
typedef const char *func_str(void);
typedef void func_mutate_expr(struct expr *);
typedef enum lvalue_kind func_is_lval(const struct expr *);
typedef int func_bool(const struct expr *);

typedef ucc_wur const out_val *func_gen(const struct expr *, out_ctx *);

struct dump;
typedef void func_dump(const struct expr *, struct dump *);

#define UNUSED_OCTX() (void)octx; return NULL

typedef struct expr expr;
struct expr
{
	where where;

	func_fold *f_fold;
	func_gen *f_gen;
	func_dump *f_dump;
	func_str *f_str;
	func_is_lval *f_islval;
	func_bool *f_has_sideeffects; /* optional */

	func_const *f_const_fold; /* optional, used in static/global init */

	int freestanding; /* e.g. 1; needs use, whereas x(); doesn't - freestanding */
	struct
	{
		int const_folded;
		consty k;
	} const_eval;

	/* flags */
	/* do we return the altered value or the old one? */
	int assign_is_post;
	int assign_is_init;
#define expr_cast_implicit assign_is_post
#define expr_is_st_dot     assign_is_post
#define expr_addr_implicit assign_is_post
#define expr_comp_lit_cgen assign_is_post
#define expr_comma_synthesized assign_is_post
	enum what_of
	{
		what_sizeof,
		what_typeof,
		what_alignof,
	} what_of;

	expr *lhs, *rhs;
	expr *expr;

	union
	{
		numeric num;

		struct
		{
			enum op_type op;
			int array_notation; /* a[b] */
		} op;

		struct
		{
			type *upcast_ty;
			enum op_type op;
		} compoundop;

		/* __builtin_va_start */
		int n;

		/* __builtin_has_attribute */
		struct
		{
			expr *expr;
			type *ty;
			char *ident;
			int alloc;
		} builtin_ident;

		struct
		{
			stringlit_at lit_at; /* for strings */
			int is_func; /* __func__ (1) / __FUNCTION__ (2) */
		} strlit;

		struct
		{
			enum
			{
				IDENT_NORM,
				IDENT_ENUM
			} type;
			union
			{
				struct
				{
					sym *sym;
					char *spel;
				} ident;
				struct enum_member *enum_mem;
			} bits;
		} ident;

		struct
		{
			char *spel;
			struct label *label;
			int static_ctx;
		} lbl;

		struct /* used in compound literal */
		{
			sym *sym;
			decl *decl;
			int static_ctx;
		} complit;

		struct
		{
			decl *d;
			unsigned extra_off;
		} struct_mem;

		struct
		{
			struct funcargs *args;
			type *retty;
			sym *sym;
		} block;

		type **types; /* used in __builtin */

		type *va_arg_type;

		type *cast_to;

		struct
		{
			unsigned sz;
			type *of_type;
			decl *vm;
		} size_of;

		struct
		{
			struct generic_lbl
			{
				type *t; /* NULL -> default */
				expr *e;
			} **list, *chosen;
		} generic;

		struct
		{
			size_t len;
			int ch;
		} builtin_memset;

		enum
		{
			builtin_nanf, builtin_nan, builtin_nanl
		} builtin_nantype;

		struct stmt *variadic_setup;

		type *offsetof_ty;
	} bits;

	int in_parens; /* for if((x = 5)) testing */

	expr **funcargs;
	struct stmt *code; /* ({ ... }), comp. lit. assignments */

	/* type propagation */
	type *tree_type;
};


expr *expr_new(
		func_mutate_expr *,
		func_fold *,
		func_str *,
		func_gen *,
		func_dump *,
		func_gen *);

void expr_mutate(
		expr *,
		func_mutate_expr *,
		func_fold *,
		func_str *,
		func_gen *,
		func_dump *,
		func_gen *);

/* sets e->where */
expr *expr_set_where(expr *, where const *);

/* sets e->where and e->where.len based on the change */
expr *expr_set_where_len(expr *, where *);

#define expr_mutate_wrapper(e, type) \
	expr_mutate(e,                \
			mutate_expr_     ## type, \
			fold_expr_       ## type, \
			str_expr_        ## type, \
			gen_expr_        ## type, \
			dump_expr_       ## type, \
			gen_expr_style_  ## type)

#define expr_new_wrapper(type)   \
	expr_new(mutate_expr_ ## type, \
			fold_expr_       ## type,  \
			str_expr_        ## type,  \
			gen_expr_        ## type,  \
			dump_expr_       ## type,  \
			gen_expr_style_  ## type)

#define EXPR_DEFS(type)                  \
	func_fold fold_expr_ ## type;          \
	func_gen gen_expr_ ## type;            \
	func_str str_expr_ ## type;            \
	func_dump dump_expr_ ## type;          \
	func_mutate_expr mutate_expr_ ## type; \
	func_gen gen_expr_style_ ## type

expr *expr_new_numeric(numeric *);

/* simple wrappers */
expr *expr_ptr_multiply(expr *, decl *);
expr *expr_new_decl_init(decl *d, struct decl_init *di);

void expr_free(expr *);
void expr_free_abi(void *);

#define expr_kind(exp, kind) ((exp)->f_str == str_expr_ ## kind)

expr *expr_compiler_generated(expr *);

enum null_strictness
{
	NULL_STRICT_VOID_PTR = 0,
	NULL_STRICT_INT = 1 << 0,
	NULL_STRICT_ANY_PTR = 1 << 1
};

int expr_is_null_ptr(expr *, enum null_strictness);

func_is_lval expr_is_lval;
func_is_lval expr_is_lval_always;
func_is_lval expr_is_lval_struct;

func_bool expr_is_struct_bitfield; /* a->b where b is bitfield */

func_bool expr_has_sideeffects;
func_bool expr_bool_always;

void expr_set_const(expr *, consty *);

/* util */
expr *expr_new_array_idx_e(expr *base, expr *idx);
expr *expr_new_array_idx(expr *base, int i);

expr *expr_skip_all_casts(expr *);
expr *expr_skip_lval2rval(expr *);
expr *expr_skip_implicit_casts(expr *);
expr *expr_skip_generated_casts(expr *);

const char *expr_str_friendly(expr *, int show_implicit_casts);

decl *expr_to_declref(expr *e, const char **whynot);
sym *expr_to_symref(expr *e, symtable * /*optional, will search if given*/);

#endif
