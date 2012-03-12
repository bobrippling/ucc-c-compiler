#ifndef OP_H
#define OP_H

typedef int         op_exec(expr *lhs, expr *rhs, int *bad);
typedef int         op_optimise(expr *lhs, expr *rhs);
typedef void        op_compare(expr *lhs, expr *rhs);
typedef void        op_fold(     expr *e);
typedef void        op_gen(      expr *e);
typedef void        op_gen_store(expr *e);

struct op
{
	op_exec       *f_exec;
	op_optimise   *f_optimise; /* optional */

	op_fold       *f_fold;
	op_gen        *f_gen, *f_gen_str;
	op_gen_store  *f_store;   /* optional */
	op_compare    *f_compare; /* optional */

	func_str      *f_str;

	expr *lhs, *rhs;
};


op   *op_new(expr *lhs, expr *rhs, op_exec *f_exec, op_fold *f_fold, op_gen *f_gen, op_gen *f_gen_str, func_str *f_str);
void  op_mutate(op *e,             op_exec *f_exec, op_fold *f_fold, op_gen *f_gen, op_gen *f_gen_str, func_str *f_str);

#include "ops/op_deref.h"
#include "ops/op_divide.h"
#include "ops/op_eq.h"
#include "ops/op_ge.h"
#include "ops/op_le.h"
#include "ops/op_minus.h"
#include "ops/op_modulus.h"
#include "ops/op_multiply.h"
#include "ops/op_not.h"
#include "ops/op_or.h"
#include "ops/op_orsc.h"
#include "ops/op_plus.h"
#include "ops/op_shiftl.h"
#include "ops/op_struct_ptr.h"
#include "ops/op_xor.h"
#include "ops/op_and.h"
#include "ops/op_andsc.h"
#include "ops/op_bnot.h"
#include "ops/op_gt.h"
#include "ops/op_lt.h"
#include "ops/op_ne.h"
#include "ops/op_shiftr.h"
#include "ops/op_struct_dot.h"

#define op_new_wrapper(type, l, r)  op_new(l, r, exec_op_ ## type, fold_op_ ## type, gen_op_ ## type, gen_str_op_ ## type, str_op_ ## type)
#define op_mutate_wrapper(e, type)  op_mutate(e, exec_op_ ## type, fold_op_ ## type, gen_op_ ## type, gen_str_op_ ## type, str_op_ ## type)

op *op_new_multiply  (expr *l, expr *r);
op *op_new_not       (expr *n);
op *op_new_bnot      (expr *n);

op *op_new_deref     (expr *l, expr *r);

op *op_from_token(enum token, expr *, expr *);

#if 0
enum op_type curtok_to_augmented_op()
{
#define CASE(x) case token_ ## x ## _assign: return op_ ## x
	switch(curtok){
		CASE(plus);
		CASE(minus);
		CASE(multiply);
		CASE(divide);
		CASE(modulus);
		CASE(not);
		CASE(bnot);
		CASE(and);
		CASE(or);
		CASE(xor);
		CASE(shiftl);
		CASE(shiftr);
		default:
			break;
	}
	return op_unknown;
#undef CASE
}
#endif


#endif
