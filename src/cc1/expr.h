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

typedef void func_fold(struct expr *, struct symtable *);
typedef void func_const(struct expr *, consty *);
typedef const char *func_str(void);
typedef void func_mutate_expr(struct expr *);
typedef int func_is_lval(struct expr *);

typedef ucc_wur const out_val *func_gen(struct expr *, out_ctx *);
typedef ucc_wur const out_val *func_gen_lea(struct expr *, out_ctx *);

#define UNUSED_OCTX() (void)octx; return NULL

typedef struct expr expr;
struct expr
{
	where where;

	func_fold *f_fold;
	func_gen *f_gen;
	func_str *f_str;

	func_const *f_const_fold; /* optional, used in static/global init */
	func_gen_lea *f_lea; /* optional */
	func_is_lval *f_islval; /* optional */

	/* not a user lvalue, e.g. a?b:c, where the operands are structs */
	int lvalue_internal;

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
		} op;

		struct
		{
			enum op_type op;
			int upcast;
		} compoundop;

		/* __builtin_va_start */
		int n;

		struct
		{
			stringlit_at lit_at; /* for strings */
			int is_func; /* __func__ ? */
		} strlit;

		struct
		{
			sym *sym;
			char *spel;
		} ident;

		struct
		{
			char *spel;
			struct label *label;
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
	} bits;

	int in_parens; /* for if((x = 5)) testing */

	expr **funcargs;
	struct stmt *code; /* ({ ... }), comp. lit. assignments */

	/* type propagation */
	type *tree_type;
};


expr *expr_new(          func_mutate_expr *, func_fold *, func_str *, func_gen *, func_gen *, func_gen *);
void expr_mutate(expr *, func_mutate_expr *, func_fold *, func_str *, func_gen *, func_gen *, func_gen *);

/* sets e->where */
expr *expr_set_where(expr *, where const *);

/* sets e->where and e->where.len based on the change */
expr *expr_set_where_len(expr *, where *);

#define expr_mutate_wrapper(e, type) expr_mutate(e,               \
                                        mutate_expr_     ## type, \
                                        fold_expr_       ## type, \
                                        str_expr_        ## type, \
                                        gen_expr_        ## type, \
                                        gen_expr_str_    ## type, \
                                        gen_expr_style_  ## type)

#define expr_new_wrapper(type) expr_new(mutate_expr_     ## type, \
                                        fold_expr_       ## type, \
                                        str_expr_        ## type, \
                                        gen_expr_        ## type, \
                                        gen_expr_str_    ## type, \
                                        gen_expr_style_  ## type)

expr *expr_new_numeric(numeric *);

/* simple wrappers */
expr *expr_ptr_multiply(expr *, decl *);
expr *expr_new_decl_init(decl *d, struct decl_init *di);

#include "ops/expr_addr.h"
#include "ops/expr_assign.h"
#include "ops/expr_assign_compound.h"
#include "ops/expr_cast.h"
#include "ops/expr_comma.h"
#include "ops/expr_funcall.h"
#include "ops/expr_identifier.h"
#include "ops/expr_if.h"
#include "ops/expr_op.h"
#include "ops/expr_sizeof.h"
#include "ops/expr_val.h"
#include "ops/expr_stmt.h"
#include "ops/expr_deref.h"
#include "ops/expr_struct.h"
#include "ops/expr_compound_lit.h"
#include "ops/expr_string.h"

/* XXX: memleak */
#define expr_free(x) do{                 \
		if(x){                               \
			/*if((x)->tree_type)*/             \
			/*type_free((x)->tree_type);*/ \
			free(x);                           \
		}                                    \
	}while(0)

#define expr_kind(exp, kind) ((exp)->f_str == str_expr_ ## kind)

expr *expr_new_identifier(char *sp);
expr *expr_new_cast(expr *, type *cast_to, int implicit);
expr *expr_new_cast_lval_decay(expr *);

expr *expr_new_identifier(char *sp);
expr *expr_new_val(int val);
expr *expr_new_op(enum op_type o);
expr *expr_new_op2(enum op_type o, expr *l, expr *r);
expr *expr_new_if(expr *test);
expr *expr_new_stmt(struct stmt *code);
expr *expr_new_sizeof_type(type *, enum what_of what_of);
expr *expr_new_sizeof_expr(expr *, enum what_of what_of);
expr *expr_new_funcall(void);
expr *expr_new_assign(         expr *to, expr *from);
expr *expr_new_assign_init(    expr *to, expr *from);
expr *expr_new_assign_compound(expr *to, expr *from, enum op_type);
expr *expr_new__Generic(expr *test, struct generic_lbl **lbls);
expr *expr_new_block(type *rt, struct funcargs *args);
expr *expr_new_deref(expr *);
expr *expr_new_struct(expr *sub, int dot, expr *ident);
expr *expr_new_struct_mem(expr *sub, int dot, decl *);
expr *expr_new_str(char *, size_t, int wide, where *, symtable *stab);
expr *expr_new_addr_lbl(char *);
expr *expr_new_addr(expr *);

expr *expr_new_comma2(expr *lhs, expr *rhs);
#define expr_new_comma() expr_new_wrapper(comma)

enum null_strictness
{
	NULL_STRICT_VOID_PTR,
	NULL_STRICT_INT,
	NULL_STRICT_ANY_PTR
};

int expr_is_null_ptr(expr *, enum null_strictness);

int expr_is_lval(expr *);

int expr_is_lval_unless_array(expr *);
int expr_is_lval_always(expr *);

void expr_set_const(expr *, consty *);

/* util */
expr *expr_new_array_idx_e(expr *base, expr *idx);
expr *expr_new_array_idx(expr *base, int i);

expr *expr_skip_casts(expr *);

#endif
