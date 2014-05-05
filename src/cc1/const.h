#ifndef CONST_H
#define CONST_H

#include "num.h"
#include "strings.h"
#include "op.h"

struct expr;


typedef struct stringlit_at
{
	where where;
	stringlit *lit;
} stringlit_at;

typedef struct consty
{
	enum constyness
	{
		CONST_NO = 0,   /* f() */
		CONST_NUM,      /* 5 + 2, float, etc */
		/* can be offset: */
		CONST_ADDR,     /* &f where f is global */
		CONST_STRK,     /* string constant */
		CONST_NEED_ADDR, /* a.x, b->y, p where p is global int p */
	} type;
	sintegral_t offset; /* offset for addr/strk */
	union
	{
		numeric num;        /* CONST_VAL_* */
		stringlit_at *str; /* CONST_STRK */
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
	struct expr *nonstandard_const; /* e.g. (1, 2) is not strictly const */
} consty;
#define CONST_AT_COMPILE_TIME(t) (t != CONST_NO && t != CONST_NEED_ADDR)

#define CONST_ADDR_OR_NEED_TREF(r)  \
	(  type_is_array(r)           \
	|| type_is_decayed_array(r)   \
	|| type_is(r, type_func)  \
		? CONST_ADDR : CONST_NEED_ADDR)

#define CONST_ADDR_OR_NEED(d) CONST_ADDR_OR_NEED_TREF((d)->ref)

#define K_FLOATING(num) !!((num).suffix & VAL_FLOATING)
#define K_INTEGRAL(num) !K_FLOATING(num)

#define CONST_FOLD_LEAF(k) memset((k), 0, sizeof *(k))

void const_fold(struct expr *e, consty *);

int const_expr_and_zero(struct expr *e);
int const_expr_and_non_zero(struct expr *e);

void const_fold_integral(struct expr *e, numeric *);
integral_t const_fold_val_i(struct expr *e);

integral_t const_op_exec(
		/* rval is optional */
		integral_t lval, const integral_t *rval,
		enum op_type op, int is_signed,
		const char **error);

floating_t const_op_exec_fp(
		floating_t lv, const floating_t *rv,
		enum op_type op);

#endif
