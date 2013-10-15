#ifndef EXPR_H
#define EXPR_H

typedef struct consty
{
	enum constyness
	{
		CONST_NO = 0,   /* f() */
		CONST_VAL,      /* 5 + 2 */
		/* can be offset: */
		CONST_ADDR,     /* &f where f is global */
		CONST_STRK,     /* string constant */
		CONST_NEED_ADDR, /* a.x, b->y, p where p is global int p */
	} type;
	long offset; /* offset for addr/strk */
	union
	{
		intval iv;          /* CONST_VAL */
		stringval *str;     /* CONST_STRK */
		struct
		{
			int is_lbl;
			union
			{
				const char *lbl;
				unsigned long memaddr;
			} bits;
		} addr;
	} bits;
	expr *nonstandard_const; /* e.g. (1, 2) is not strictly const */
} consty;
#define CONST_AT_COMPILE_TIME(t) (t != CONST_NO && t != CONST_NEED_ADDR)

#define CONST_ADDR_OR_NEED_TREF(r)  \
	(  type_ref_is_array(r)           \
	|| type_ref_is_decayed_array(r)   \
	|| type_ref_is(r, type_ref_func)  \
		? CONST_ADDR : CONST_NEED_ADDR)

#define CONST_ADDR_OR_NEED(d) CONST_ADDR_OR_NEED_TREF((d)->ref)

#define CONST_FOLD_LEAF(k) memset((k), 0, sizeof *(k))


typedef void         func_fold(          expr *, symtable *);
typedef void         func_gen(           expr *);
typedef void         func_gen_lea(       expr *);
typedef void         func_const(         expr *, consty *);
typedef const char  *func_str(void);
typedef void         func_mutate_expr(expr *);

struct expr
{
	where where;

	func_fold        *f_fold;
	func_gen         *f_gen;
	func_str         *f_str;

	func_const       *f_const_fold; /* optional, used in static/global init */
	func_gen_lea     *f_lea;        /* optional */


	int freestanding; /* e.g. 1; needs use, whereas x(); doesn't - freestanding */
	struct
	{
		int const_folded;
		consty k;
	} const_eval;

	enum op_type op;

	/* flags */
	/* do we return the altered value or the old one? */
	int assign_is_post;
	int assign_is_init;
#define expr_is_default    assign_is_post
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
		intval iv;

		/* __builtin_va_start */
		int n;

		struct
		{
			sym *sym;
			stringval sv; /* for strings */
		} str;

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
		} complit;

		struct
		{
			decl *d;
			unsigned extra_off;
		} struct_mem;

		struct
		{
			funcargs *args;
			type_ref *retty;
			sym *sym;
		} block;

		type_ref **types; /* used in __builtin */

		type_ref *tref; /* from cast */

		struct
		{
			unsigned sz;
			type_ref *of_type;
		} size_of;

		struct
		{
			struct generic_lbl
			{
				type_ref *t; /* NULL -> default */
				expr *e;
			} **list, *chosen;
		} generic;

		struct
		{
			size_t len;
			int ch;
		} builtin_memset;

		stmt *variadic_setup;
	} bits;

	int in_parens; /* for if((x = 5)) testing */

	expr **funcargs;
	stmt *code; /* ({ ... }), comp. lit. assignments */

	/* type propagation */
	type_ref *tree_type;
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

expr *expr_new_intval(intval *);

/* simple wrappers */
expr *expr_ptr_multiply(expr *, decl *);
expr *expr_new_decl_init(decl *d, decl_init *di);

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
			/*type_ref_free((x)->tree_type);*/ \
			free(x);                           \
		}                                    \
	}while(0)

#define expr_kind(exp, kind) ((exp)->f_str == str_expr_ ## kind)

expr *expr_new_identifier(char *sp);
expr *expr_new_cast(expr *, type_ref *cast_to, int implicit);
expr *expr_new_val(int val);
expr *expr_new_op(enum op_type o);
expr *expr_new_op2(enum op_type o, expr *l, expr *r);
expr *expr_new_if(expr *test);
expr *expr_new_stmt(stmt *code);
expr *expr_new_sizeof_type(type_ref *, enum what_of what_of);
expr *expr_new_sizeof_expr(expr *, enum what_of what_of);
expr *expr_new_funcall(void);
expr *expr_new_assign(         expr *to, expr *from);
expr *expr_new_assign_init(    expr *to, expr *from);
expr *expr_new_assign_compound(expr *to, expr *from, enum op_type);
expr *expr_new__Generic(expr *test, struct generic_lbl **lbls);
expr *expr_new_block(type_ref *rt, funcargs *args, stmt *code);
expr *expr_new_deref(expr *);
expr *expr_new_struct(expr *sub, int dot, expr *ident);
expr *expr_new_struct_mem(expr *sub, int dot, decl *);
expr *expr_new_str(char *, int len, int wide);
expr *expr_new_addr_lbl(char *);
expr *expr_new_addr(expr *);

expr *expr_new_comma2(expr *lhs, expr *rhs);
#define expr_new_comma() expr_new_wrapper(comma)

int expr_is_null_ptr(expr *, int allow_int);

/* util */
expr *expr_new_array_idx_e(expr *base, expr *idx);
expr *expr_new_array_idx(expr *base, int i);

#endif
